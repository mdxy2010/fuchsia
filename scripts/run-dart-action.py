#!/usr/bin/env python
# Copyright 2016 The Fuchsia Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import json
import multiprocessing
import os
import paths
import Queue
import subprocess
import sys
import threading


def gn_describe(out, path):
    gn = os.path.join(paths.FUCHSIA_ROOT, 'buildtools', 'gn')
    data = subprocess.check_output(
        [gn, 'desc', out, path, '--format=json'], cwd=paths.FUCHSIA_ROOT)
    return json.loads(data)


class WorkerThread(threading.Thread):
    '''
    A worker thread to run scripts from a queue and return exit codes and output
    on a queue.
    '''

    def __init__(self, script_queue, result_queue, args):
        threading.Thread.__init__(self)
        self.script_queue = script_queue
        self.result_queue = result_queue
        self.args = args
        self.daemon = True

    def run(self):
        while True:
            try:
                script = self.script_queue.get(False)
            except Queue.Empty, e:
                # No more scripts to run.
                return
            if not os.path.exists(script):
                self.result_queue.put((script, -1, 'Script does not exist.'))
                continue
            job = subprocess.Popen(
                [script] + self.args,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE)
            stdout, stderr = job.communicate()
            self.result_queue.put((script, job.returncode, stdout + stderr))


def main():
    parser = argparse.ArgumentParser(
        '''Run Dart actions (analysis, tests) for Dart build targets.
Extra flags will be passed to the supporting Dart tool if applicable.
''')
    parser.add_argument(
        '--out',
        help='Path to the base output directory, e.g. out/debug-x86-64',
        required=True)
    parser.add_argument(
        '--tree',
        help='Restrict analysis to a source subtree, e.g. //topaz/shell/*',
        default='*')
    parser.add_argument(
        '--jobs', '-j',
        help='Number of concurrent instances to run',
        type=int,
        default=multiprocessing.cpu_count())
    parser.add_argument(
        '--verbose', '-v',
        help='Show output from tests that pass',
        action='store_true')
    parser.add_argument(
        'action',
        help='Action to perform on the targets',
        choices=('analyze', 'test'))
    args, extras = parser.parse_known_args()

    if not os.path.isdir(os.path.join(paths.FUCHSIA_ROOT, args.out)):
        print 'Invalid output directory: %s' % args.out
        return 1

    tree = args.tree
    if args.action == 'analyze':
        tree = '%s(//build/dart/config:dartlang)' % tree

    # Ask gn about all the dart analyzer scripts.
    scripts = []
    targets = gn_describe(args.out, tree)
    if not targets:
        print 'No targets found...'
        exit(1)
    target_script = ('//build/dart/gen_analyzer_invocation.py'
                     if args.action == 'analyze'
                     else '//build/dart/gen_test_invocation.py')
    for target_name, properties in targets.items():
        if ('type' not in properties or
                properties['type'] != 'action' or
                'script' not in properties or
                properties['script'] != target_script or
                'outputs' not in properties or
                not len(properties['outputs'])):
            continue
        script_path = properties['outputs'][0]
        script_path = script_path[2:]  # Remove the leading //
        scripts.append(os.path.join(paths.FUCHSIA_ROOT, script_path))

    # Put all the analyzer scripts in a queue that workers will work from
    script_queue = Queue.Queue()
    for script in scripts:
        script_queue.put(script)
    # Make a queue to receive results from workers.
    result_queue = Queue.Queue()
    # Track return codes from scripts.
    script_results = []
    failed_scripts = []

    # Create a worker thread for each CPU on the machine.
    for i in range(args.jobs):
        WorkerThread(script_queue, result_queue, extras).start()

    def print_progress():
        sys.stdout.write('\rProgress: %d/%d\033[K' % (len(script_results),
                                                      len(scripts)))
        sys.stdout.flush()

    print_progress()

    # Handle results from workers.
    while len(script_results) < len(scripts):
        script, returncode, output = result_queue.get(True)
        script_results.append(returncode)
        print_progress()
        if returncode != 0:
            failed_scripts.append(script)
        if args.verbose or returncode != 0:
            print '\r----------------------------------------------------------'
            print script
            print output

    print ''
    if len(failed_scripts):
        failed_scripts.sort()
        print 'Failures in:'
        for script in failed_scripts:
            print '  %s' % script
        exit(1)


if __name__ == '__main__':
    sys.exit(main())
