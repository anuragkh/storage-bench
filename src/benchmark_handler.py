from __future__ import print_function

import os
import shutil
import socket
import subprocess
import sys

import boto3
from six.moves import configparser

if sys.version_info[0] < 3:
    def b(x):
        return x


    def bytes_to_str(x):
        return x
else:
    def b(x):
        return x.encode('utf-8') if not isinstance(x, bytes) else x


    def bytes_to_str(x):
        return x.decode('utf-8') if isinstance(x, bytes) else x


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
        self.f.send(b('CLOSE'))
        self.f.shutdown(socket.SHUT_RDWR)
        self.f.close()


def _init_bin(bin_path):
    if not os.path.exists('/tmp/bin'):
        os.makedirs('/tmp/bin')
    in_path = os.environ.get('LAMBDA_TASK_ROOT', bin_path)
    out_path = os.environ.get('TMP_PATH', '/tmp/bin')
    cur_file = os.path.join(in_path, 'storage_bench')
    new_file = os.path.join(out_path, 'storage_bench')
    if not os.path.isfile(new_file):
        shutil.copy2(cur_file, new_file)
        os.chmod(new_file, 0o775)
    return new_file


def _copy_results(logger, prefix, result):
    bucket = boto3.resource('s3').Bucket('bench-results')
    if os.path.isfile(result):
        logger.info('Copying results @ {} to S3...'.format(result))
        with open(result, 'rb') as data:
            bucket.put_object(Key=prefix + "_" + os.path.basename(result), Body=data)


def _run_benchmark(logger, system, conf, out, bench, num_ops, warm_up, mode, dist, bin_path):
    executable = _init_bin(bin_path)
    cmdline = [executable, system, conf, out, str(bench), mode, str(num_ops), str(warm_up), dist]
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
    mode = event.get('mode')
    lambda_id = event.get('id')
    bin_path = event.get('bin_path')
    object_size = event.get('object_size')
    num_ops = event.get('num_ops')
    warm_up = event.get('warm_up')
    dist = event.get('dist')
    result_suffixes = ['_read_latency.txt', '_read_throughput.txt', '_write_latency.txt', '_write_throughput.txt']

    try:
        logger = _connect_logger(host, port)
    except Exception as e:
        print('Exception: {}'.format(e))
        raise

    logger.info('Event: {}, Context: {}'.format(event, context))

    prefix = os.path.join('/tmp', system + '_' + lambda_id)
    conf_file = prefix + '.conf'
    try:
        _create_ini(logger, system, conf, conf_file)
        _run_benchmark(logger, system, conf_file, prefix, object_size, num_ops, warm_up, mode, dist, bin_path)
    except Exception as e:
        logger.error(e)
    finally:
        for result_suffix in result_suffixes:
            _copy_results(logger, os.path.join(system, lambda_id), prefix + '_' + str(object_size) + result_suffix)

    logger.close()
