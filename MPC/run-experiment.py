import argparse
import subprocess
import shutil
import os
import sys
import time

def run(cmd):
    return subprocess.run(cmd, stdout=subprocess.PIPE, check=True, text=True, shell=True).stdout.strip()

def move_target_files(args, target):
    # collect bytecode and schedule files
    bytecode_files = run(f'ls -1 {target}-*.bc')
    bytecode_files = [path.strip() for path in bytecode_files.splitlines() if path.strip() != '']
    if len(bytecode_files) == 0:
        raise Error(f"Cannot find bytecode files for target '{target}'")
    for path in bytecode_files:
        shutil.copy(path, f'{args.mpspdzhome}/Programs/Bytecode/')
    schedule_file = run(f'ls -1 {target}.sch')
    shutil.copy(schedule_file, f'{args.mpspdzhome}/Programs/Schedules/')

def as_logname(s, playerid, dir=None):
    s = s.split('.')
    prefix = f'{dir}/' if dir is not None else ''
    if len(s) == 1:
        return f'{prefix}{s[0]}{playerid}.log'
    else:
        s = '.'.join(s[:-1])
        return f'{prefix}{s}{playerid}.log'

def benchmark(args, target, playerid):
    # create log dir
    run(f'mkdir -p {target}')
    # create target file
    with open(f'{args.mpspdzhome}/target', 'w') as fp:
        fp.write(target)
    # run both script
    both_cmd = f'sh {args.both} > {as_logname(args.both, playerid, dir=target)} 2>&1'
    print(f'Running both script: {both_cmd}')
    run(both_cmd)
    print('\tDone')

parser = argparse.ArgumentParser(description='Run experiment',formatter_class=argparse.ArgumentDefaultsHelpFormatter)
parser.add_argument('--mp-spdz', dest='mpspdzhome', required=False, type=str, help='Path to root directory of MP-SPDZ', default='MP-SPDZ')
parser.add_argument('--both', dest='both', required=False, type=str, help='Path to both script', default='run-both.sh')
parser.add_argument('--skip', dest='skip', required=False, action='store_true', help='Skip setup and moving target files', default=False)
parser.add_argument('--detach', dest='detach', required=False, action='store_true', help='Start benchmark detached', default=False)
parser.add_argument('targets', metavar='target', type=str, nargs='+', help='Targets to benchmark')
args = parser.parse_args()

if not args.skip:
    # check that scripts exist
    if not os.path.isfile(args.both):
        raise Error(f'Cannot find both script {args.both}')

    print(f'Moving target files: {" ".join(args.targets)}')
    for target in args.targets:
        move_target_files(args, target)
    print('\tDone')

with open(f'{args.mpspdzhome}/playerid', 'r') as fp:
    playerid = int(fp.read().strip())

# detach child process that will run the remaining benchmark
if args.detach:
    targets = ' '.join(args.targets)
    print('Starting detached benchmark. Bye')
    cmd = f"python3 -u {sys.argv[0]} --skip --mp-spdz '{args.mpspdzhome}' --both '{args.both}' {targets}"
    run(f'tmux new-session -d -s benchmark "{cmd}"')
else:
    for target in args.targets:
        print(f'Starting benchmark for {target}')
        benchmark(args, target, playerid)
        print(f'Stopping benchmark for {target}\n\n')
        print('Waiting 5s for all players to wrap up.')
        time.sleep(5)
    # tar result
    run(f'tar -cvf res{playerid}.tar {" ".join(args.targets)}')
