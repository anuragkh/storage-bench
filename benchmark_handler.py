from __future__ import print_function

import os
import shutil
import subprocess
import time
from six.moves import configparser

import boto3

LAMBDA_TASK_ROOT = os.environ.get('LAMBDA_TASK_ROOT', os.path.dirname(os.path.abspath(__file__)))
CUR_BIN_DIR = os.path.join(LAMBDA_TASK_ROOT, 'bin')
LIB_DIR = os.path.join(LAMBDA_TASK_ROOT, 'lib')
BIN_DIR = '/tmp/bin'
EXECUTABLE = 'storage_bench'


def _init_bin(executable_name):
    start = time.clock()
    if not os.path.exists(BIN_DIR):
        print("Creating bin folder")
        os.makedirs(BIN_DIR)
    print("Copying binaries for " + executable_name + " in /tmp/bin")
    cur_file = os.path.join(CUR_BIN_DIR, executable_name)
    new_file = os.path.join(BIN_DIR, executable_name)
    shutil.copy2(cur_file, new_file)
    print("Giving new binaries permissions for lambda")
    os.chmod(new_file, 0o775)
    elapsed = (time.clock() - start)
    print(executable_name + " ready in " + str(elapsed) + 's.')


def _copy_results(result):
    bucket = boto3.resource('s3').Bucket('bench-results')
    with open(result, 'rb') as data:
        bucket.put_object(Key=result, Body=data)


def _run_benchmark(system, conf, out, bench):
    _init_bin(EXECUTABLE)
    cmdline = [os.path.join(BIN_DIR, EXECUTABLE), system, conf, out, bench]
    subprocess.check_call(cmdline, shell=False, stderr=subprocess.STDOUT)


def _create_ini(system, conf, out):
    config = configparser.ConfigParser()
    config.add_section(system)
    for key in conf.keys():
        config.set(system, key, conf[key])
    with open(out, 'w') as f:
        conf.write(f)

def benchmark_handler(event, context):
    system = context.client_context.system
    conf = context.client_context.conf
    prefix = os.path.join('/tmp', system)
    _create_ini(system, conf, prefix + '.conf')
    _run_benchmark(system, prefix + '.conf', prefix, 'sweep')
