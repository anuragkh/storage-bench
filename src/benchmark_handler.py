from __future__ import print_function

import os
import shutil
import socket
import subprocess

import boto3
from six.moves import configparser


class Logger(object):
    def __init__(self, f):
        self.f = f

    def info(self, msg):
        self._log('INFO', msg)

    def warn(self, msg):
        self._log('WARN', msg)

    def error(self, msg):
        self._log('ERROR', msg)

    def write(self, msg):
        self.info(msg)

    def _log(self, msg_type, msg):
        self.f.send('{} {}'.format(msg_type, msg))


def _init_bin(bin_path):
    if not os.path.exists('/tmp/bin'):
        os.makedirs('/tmp/bin')
    bin_path = os.environ.get('LAMBDA_TASK_ROOT', bin_path)
    cur_file = os.path.join(bin_path, 'storage_bench')
    new_file = os.path.join('/tmp/bin', 'storage_bench')
    shutil.copy2(cur_file, new_file)
    os.chmod(new_file, 0o775)
    return new_file


def _copy_results(logger, result):
    logger.info('Copying results @ {}'.format(result))
    bucket = boto3.resource('s3').Bucket('bench-results')
    with open(result, 'rb') as data:
        bucket.put_object(Key=result, Body=data)


def _run_benchmark(logger, system, conf, out, bench, bin_path):
    executable = _init_bin(bin_path)
    cmdline = [executable, system, conf, out, bench]
    logger.info('Running benchmark, cmd: {}'.format(cmdline))
    subprocess.check_call(cmdline, shell=False, stderr=logger, stdout=logger)


def _create_ini(logger, system, conf, out):
    config = configparser.ConfigParser()
    config.add_section(system)
    for key in conf.keys():
        config.set(system, key, conf[key])
    with open(out, 'w') as f:
        config.write(f)
    logger.info('Created configuration file {}'.format(out))


def _connect_logger(host, port):
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((host, port))
    return Logger(sock)


def benchmark_handler(event, context):
    system = event['system']
    conf = event['conf']
    host = event['host']
    port = event['port']
    bin_path = event['bin_path']

    try:
        logger = _connect_logger(host, port)
    except Exception as e:
        print('Exception: {}'.format(e))
        raise

    logger.info('System: {}, conf: {}, logger.host: {}, logger.port: {}'.format(system, conf, host, port))

    prefix = os.path.join('/tmp', system)
    _create_ini(logger, system, conf, prefix + '.conf')
    _run_benchmark(logger, system, prefix + '.conf', prefix, 'sweep', bin_path)
