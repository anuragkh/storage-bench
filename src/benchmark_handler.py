from __future__ import print_function

import os
import shutil
import socket
import subprocess
import sys

import boto3
from six.moves import configparser

if sys.version_info[0] < 3:
    try:
        from cStringIO import StringIO as BytesIO
    except ImportError:
        from StringIO import StringIO as BytesIO


    def safe_unicode(obj, *args):
        """ return the unicode representation of obj """
        try:
            return unicode(obj, *args)
        except UnicodeDecodeError:
            # obj is byte string
            ascii_text = str(obj).encode('string_escape')
            return unicode(ascii_text)


    def iteritems(x):
        return x.iteritems()


    def iterkeys(x):
        return x.iterkeys()


    def itervalues(x):
        return x.itervalues()


    def nativestr(x):
        return x if isinstance(x, str) else x.encode('utf-8', 'replace')


    def u(x):
        return x.decode()


    def b(x):
        return x


    def next(x):
        return x.next()


    def byte_to_chr(x):
        return x


    def char_to_byte(x):
        return x


    def bytes_to_str(x):
        return x


    unichr = unichr
    xrange = xrange
    basestring = basestring
    unicode = unicode
    bytes = str
    long = long
else:
    def iteritems(x):
        return iter(x.items())


    def iterkeys(x):
        return iter(x.keys())


    def itervalues(x):
        return iter(x.values())


    def byte_to_chr(x):
        return chr(x)


    def char_to_byte(x):
        return ord(x)


    def nativestr(x):
        return x if isinstance(x, str) else x.decode('utf-8', 'replace')


    def bytes_to_str(x):
        return x.decode()


    def u(x):
        return x


    def b(x):
        return x.encode('latin-1') if not isinstance(x, bytes) else x


    next = next
    unichr = chr
    imap = map
    izip = zip
    xrange = range
    basestring = str
    unicode = str
    safe_unicode = str
    bytes = bytes
    long = int


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
    bin_path = os.environ.get('LAMBDA_TASK_ROOT', bin_path)
    cur_file = os.path.join(bin_path, 'storage_bench')
    new_file = os.path.join('/tmp/bin', 'storage_bench')
    shutil.copy2(cur_file, new_file)
    os.chmod(new_file, 0o775)
    return new_file


def _copy_results(logger, result):
    logger.info('Copying results @ {} to S3...'.format(result))
    bucket = boto3.resource('s3').Bucket('bench-results')
    if os.path.isfile(result):
        with open(result, 'rb') as data:
            bucket.put_object(Key=result, Body=data)
    else:
        logger.warn('Result {} not found'.format(result))


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
    result_suffixes = ['_read_latency.txt', '_read_throughput.txt', '_write_latency.txt', '_write_throughput.txt']

    try:
        logger = _connect_logger(host, port)
    except Exception as e:
        print('Exception: {}'.format(e))
        raise

    logger.info('Event: {}, Context: {}'.format(event, context))

    try:
        prefix = os.path.join('/tmp', system)
        _create_ini(logger, system, conf, prefix + '.conf')
        for object_size in object_sizes:
            _run_benchmark(logger, system, prefix + '.conf', prefix, str(object_size), bin_path)
    except Exception as e:
        logger.error(e)
    finally:
        prefix = os.path.join('/tmp', system)
        for object_size in object_sizes:
            for result_suffix in result_suffixes:
                _copy_results(logger, prefix + '_' + str(object_size) + result_suffix)

    logger.close()
