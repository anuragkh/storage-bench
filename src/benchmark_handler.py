from __future__ import print_function

import os
import shutil
import socket
import subprocess

import boto3
from six.moves import configparser

from src.compat import b


class NetworkFile(object):
    def __init__(self, f):
        self.f = f

    def fileno(self):
        return self.f.fileno()


class Logger(object):
    def __init__(self, f):
        self.f = f

    def info(self, msg):
        self._log('INFO', msg)

    def warn(self, msg):
        self._log('WARN', msg)

    def error(self, msg):
        self._log('ERROR', msg)

    def stdout(self):
        return NetworkFile(self.f)

    def stderr(self):
        return NetworkFile(self.f)

    def _log(self, msg_type, msg):
        self.f.send(b('{} {}'.format(msg_type, msg).rstrip()))

    def close(self):
        self.f.send('CLOSE')
        self.f.shutdown(socket.SHUT_RDWR)
        self.f.close()


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
    logger.info('Copying results @ {} to S3...'.format(result))
    bucket = boto3.resource('s3').Bucket('bench-results')
    with open(result, 'rb') as data:
        bucket.put_object(Key=result, Body=data)


def _run_benchmark(logger, system, conf, out, bench, bin_path):
    executable = _init_bin(bin_path)
    cmdline = [executable, system, conf, out, bench]
    logger.info('Running benchmark, cmd: {}'.format(cmdline))
    subprocess.check_call(cmdline, shell=False, stderr=logger.stderr(), stdout=logger.stdout())


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
    system = event.get('system')
    conf = event.get('conf')
    host = event.get('host')
    port = int(event.get('port'))
    bin_path = event.get('bin_path', '.')
    object_sizes = [8, 32, 128, 512, 2048, 8192, 32768, 131072, 524288, 2097152, 8388608, 33554432, 134217728]
    object_sizes = event.get('object_sizes', object_sizes)

    try:
        logger = _connect_logger(host, port)
    except Exception as e:
        print('Exception: {}'.format(e))
        raise

    logger.info('Event: {}, Context: {}'.format(event, context))

    try:
        prefix = os.path.join('/tmp', system)
        _create_ini(logger, system, conf, prefix + '.conf')
        result_suffixes = ['_read_latency.txt', '_read_throughput.txt', '_write_latency.txt', '_write_throughput.txt']
        for object_size in object_sizes:
            _run_benchmark(logger, system, prefix + '.conf', prefix, str(object_size), bin_path)
            for result_suffix in result_suffixes:
                _copy_results(logger, prefix + '_' + str(object_size) + result_suffix)
    except Exception as e:
        logger.error(e)

    logger.close()
