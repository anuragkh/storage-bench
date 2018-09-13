from __future__ import print_function

import os
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

    def abort(self, msg):
        self.f.send('ABORT:{}'.format(msg))
        self.f.shutdown(socket.SHUT_RDWR)
        self.f.close()


def _benchmark_binary(bin_path, executable):
    return os.path.join(os.environ.get('LAMBDA_TASK_ROOT', bin_path), executable)


def _copy_results(logger, system, result):
    bucket = boto3.resource('s3').Bucket('bench-results')
    if os.path.isfile(result):
        logger.info('Copying results @ {} to S3...'.format(result))
        with open(result, 'rb') as data:
            bucket.put_object(Key=os.path.join(system, os.path.basename(result)), Body=data)


def _run_benchmark(logger, bench_type, lambda_id, system, conf, out, bench, num_ops, warm_up, mode, dist, bin_path):
    executable = _benchmark_binary(bin_path, bench_type)
    if bench_type == 'storage_bench':
        cmdline = [executable, lambda_id, system, conf, out, str(bench), mode, str(num_ops), str(warm_up), dist]
    elif bench_type == 'notification_bench':
        cmdline = [executable, lambda_id, system, conf, out, str(bench), mode, str(num_ops)]
    else:
        raise RuntimeError('Invalid benchmark type: {}'.format(bench_type))
    logger.info('Running benchmark, cmd: {}'.format(cmdline))
    try:
        subprocess.check_call(cmdline, shell=False, stderr=logger.stderr(), stdout=logger.stdout())
    except subprocess.CalledProcessError as e:
        logger.warn('Process did not terminate cleanly (Exit code: {})'.format(e.returncode))


def _create_ini(logger, system, sys_conf, bench_conf, out):
    config = configparser.ConfigParser()
    config.add_section(system)
    for key in sys_conf.keys():
        config.set(system, key, sys_conf[key])
    config.add_section('benchmark')
    for key in bench_conf.keys():
        config.set('benchmark', key, bench_conf[key])
    with open(out, 'w') as f:
        config.write(f)
    logger.info('Created configuration file {}'.format(out))


def _connect_logger(host, port):
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((host, port))
    return Logger(sock)


def benchmark_handler(event, context):
    bench_type = event.get('bench_type')
    system = event.get('system')
    sys_conf = event.get('conf')
    bench_conf = event.get('bench_conf')
    host = event.get('host')
    log_port = int(event.get('port'))
    mode = event.get('mode')
    i = event.get('id')
    bin_path = event.get('bin_path')
    object_size = event.get('object_size')
    num_ops = event.get('num_ops')
    warm_up = event.get('warm_up')
    dist = event.get('dist')
    result_suffixes = ['_read_latency.txt', '_read_throughput.txt', '_write_latency.txt', '_write_throughput.txt']

    try:
        logger = _connect_logger(host, log_port)
    except Exception as e:
        print('Exception: {}'.format(e))
        raise

    logger.info('Event: {}, Context: {}'.format(event, context))

    prefix = os.path.join('/tmp', system + '_' + i)
    conf_file = prefix + '.conf'
    try:
        _create_ini(logger, system, sys_conf, bench_conf, conf_file)
        _run_benchmark(logger, bench_type, i, system, conf_file, prefix, object_size, num_ops, warm_up, mode, dist, bin_path)
    except Exception as e:
        logger.error(e)
        logger.abort(e)
    finally:
        for result_suffix in result_suffixes:
            _copy_results(logger, system, prefix + '_' + str(object_size) + result_suffix)
        logger.close()
