from __future__ import print_function

import argparse
import datetime
import errno
import json
import multiprocessing
import os
import select
import socket
import sys
import time
from multiprocessing import Process

import boto3
from botocore.exceptions import ClientError
from six.moves import configparser

from src import benchmark_handler
from src.benchmark_handler import bytes_to_str, b

iam_client = boto3.client('iam')
lambda_client = boto3.client('lambda')
function_name = 'StorageBenchmark'

NUM_OPS = 2000
MAX_DATA_SET_SIZE = 2147483648


def num_ops(system, value_size):
    if system == "dynamodb":
        return min(2 * NUM_OPS, int(MAX_DATA_SET_SIZE / value_size))
    elif system == "redis" or system == "mmux":
        return min(50 * NUM_OPS, int(MAX_DATA_SET_SIZE / value_size))
    return min(NUM_OPS, int(MAX_DATA_SET_SIZE / value_size))


def create_function(name):
    env = dict()
    lambda_zip = '/tmp/build/lambda.zip'
    if not os.path.isfile(lambda_zip):
        code_path = os.path.dirname(os.path.realpath(__file__))
        num_cpu = multiprocessing.cpu_count()
        os.system('mkdir -p /tmp/build && cd /tmp/build && cmake {} && make -j {} pkg'.format(code_path, num_cpu))
    with open(lambda_zip, 'rb') as f:
        zipped_code = f.read()
    role = iam_client.get_role(RoleName='aws-lambda-execute')
    resp = lambda_client.create_function(
        FunctionName=name,
        Description='Storage Benchmark',
        Runtime='python3.6',
        Role=role['Role']['Arn'],
        Handler='benchmark_handler.benchmark_handler',
        Code=dict(ZipFile=zipped_code),
        Timeout=300,
        MemorySize=3008,
        Environment=dict(Variables=env),
    )
    print('Created function: {}'.format(resp))


def parse_ini(section, conf_file):
    config = configparser.ConfigParser()
    config.read(conf_file)
    return dict(config.items(section))


def invoke_lambda(e):
    lambda_client.invoke(FunctionName=function_name, InvocationType='Event', Payload=json.dumps(e))


def invoke_locally(e):
    f = Process(target=benchmark_handler.benchmark_handler, args=(e, None,))
    f.start()
    return f


def invoke(args, mode, warm_up, lambda_id=str(0)):
    e = dict(
        system=args.system,
        conf=parse_ini(args.system, args.conf),
        bench_conf=parse_ini("benchmark", args.conf),
        host=args.host,
        port=args.port,
        bin_path=args.bin_path,
        object_size=args.obj_size,
        num_ops=args.num_ops,
        dist=args.dist,
        warm_up=warm_up,
        mode=mode,
        id=lambda_id
    )
    if args.invoke:
        return invoke_lambda(e)
    elif args.invoke_local:
        return invoke_locally(e)


def invoke_n(args, mode, n, base=0):
    return [invoke(args, mode, 0, str(base + i)) for i in range(n)]


def invoke_n_periodically(args, mode, n, period, num_periods):
    p = []
    for i in range(num_periods):
        p.extend(invoke_n(args, mode, n, i * n))
        time.sleep(period)
    return p


def is_socket_valid(socket_instance):
    """ Return True if this socket is connected. """
    if not socket_instance:
        return False

    try:
        socket_instance.getsockname()
    except socket.error as err:
        err_type = err.args[0]
        if err_type == errno.EBADF:
            return False

    try:
        socket_instance.getpeername()
    except socket.error as err:
        err_type = err.args[0]
        if err_type in [errno.EBADF, errno.ENOTCONN]:
            return False

    return True


def run_server(host, port):
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    s.setblocking(False)
    s.settimeout(300)
    try:
        s.bind((host, port))
    except socket.error as ex:
        print('Bind failed: {}'.format(ex))
        sys.exit()
    s.listen(5)
    return s


def print_logs(r, msg):
    for line in msg.splitlines():
        print('== Function @ {} {} {} =='.format(r.getpeername(), datetime.datetime.now(), line))


def log_worker(s, num_connections=1, log=True):
    inputs = [s]
    outputs = []
    n_closed = 0
    while inputs:
        readable, writable, exceptional = select.select(inputs, outputs, inputs)
        for r in readable:
            if r is s:
                sock, address = r.accept()
                sock.setblocking(False)
                inputs.append(sock)
            else:
                data = r.recv(4096)
                msg = bytes_to_str(data.rstrip().lstrip())
                if 'CLOSE' in msg or not data:
                    print_logs(r, msg.replace('CLOSE', ''))
                    inputs.remove(r)
                    r.close()
                    n_closed += 1
                    if n_closed == num_connections:
                        inputs.remove(s)
                        s.close()
                else:
                    if log:
                        print_logs(r, msg)


def control_worker(s, workers_per_trigger=1, trigger_count=1, trigger_period=0, log=True):
    inputs = [s]
    outputs = []
    ready = []
    connected = set()
    while inputs:
        readable, writable, exceptional = select.select(inputs, outputs, inputs)
        for r in readable:
            if r is s:
                sock, address = r.accept()
                sock.setblocking(False)
                inputs.append(sock)
            else:
                data = r.recv(4096)
                msg = bytes_to_str(data.rstrip().lstrip())
                lambda_id = msg.split('READY:')[1]
                if log:
                    print('... Function id={} ready ...'.format(lambda_id))
                if lambda_id not in connected:
                    if log:
                        print('... Queuing function id={} ...'.format(lambda_id))
                    connected.add(lambda_id)
                    ready.append((lambda_id, r))
                    if len(connected) == workers_per_trigger * trigger_count:
                        print('.. All queued ..')
                        inputs.remove(s)
                        s.close()
                    else:
                        print('.. Progress {}/{}'.format(len(connected), workers_per_trigger * trigger_count))
                else:
                    if log:
                        print('... Aborting function id={} ...'.format(lambda_id))
                    r.send(b('ABORT'))
                    inputs.remove(r)
                    r.close()

    ready.sort(key=lambda x: x[0])
    last_wave_ts = -1
    for t in range(trigger_count):
        elapsed = time.time() - last_wave_ts
        if last_wave_ts != -1 and elapsed < trigger_period:
            print('... Sleeping for {}s ...'.format(trigger_period - elapsed))
            time.sleep(trigger_period - elapsed)
        for idx in range(t * workers_per_trigger, (t + 1) * workers_per_trigger):
            i, sock = ready[idx]
            if not log:
                print('... Running function id={} ...'.format(i))
            sock.send(b('RUN'))
        last_wave_ts = time.time()


def log_process(host, port, num_loggers=1, log=True):
    s = run_server(host, port)
    if log:
        print('... Log server listening on {}:{} ...'.format(host, port))
    p = Process(target=log_worker, args=(s, num_loggers, log))
    p.start()
    return p


def control_process(host, port, workers_per_trigger=1, trigger_count=1, trigger_period=0, log=True):
    s = run_server(host, port)
    if log:
        print('... Control server listening on {}:{} ...'.format(host, port))
    p = Process(target=control_worker, args=(s, workers_per_trigger, trigger_count, trigger_period, log))
    p.start()
    return p


def main():
    m_help = ('\n\nmode should contain one or more of the following components,\n'
              'separated by any non-whitespace delimiter:\n'
              '\tcreate   - create the table/bucket/file\n'
              '\tread     - execute read benchmark\n'
              '\twrite    - execute write benchmark\n'
              '\tdestroy  - destroy the table/bucket/file\n'
              '\tasync{n} - send asynchronous read/write requests at rate n\n\n'
              'Examples:\n'
              '\tcreate_write_destroy_async{10} - Create table/bucket/file,\n'
              '\texecute read benchmark sending async requests at 10 op/s,\n'
              '\tand finally destroy the table/bucket/file.')
    parser = argparse.ArgumentParser(description='Run storage benchmark on AWS Lambda.',
                                     formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument('--create', action='store_true', help='create AWS Lambda function')
    parser.add_argument('--invoke', action='store_true', help='invoke AWS Lambda function')
    parser.add_argument('--invoke-local', action='store_true', help='invoke AWS Lambda function locally')
    parser.add_argument('--quiet', action='store_true', help='Suppress function logs')
    parser.add_argument('--quieter', action='store_true', help='Suppress all logs')
    parser.add_argument('--system', type=str, default='s3', help='system to benchmark')
    parser.add_argument('--conf', type=str, default='conf/storage_bench.conf', help='configuration file')
    parser.add_argument('--host', type=str, default=socket.gethostname(), help='name of host where script is run')
    parser.add_argument('--port', type=int, default=8888, help='port that server listens on')
    parser.add_argument('--num-ops', type=int, default=-1, help='number of operations')
    parser.add_argument('--bin-path', type=str, default='build', help='location of executable (local mode only)')
    parser.add_argument('--obj-size', type=int, default=8, help='object size to benchmark for')
    parser.add_argument('--dist', type=str, default='sequential', help='key distribution')
    parser.add_argument('--mode', type=str, default='create_read_write_destroy', help='benchmark mode' + m_help)
    args = parser.parse_args()

    if args.create:
        try:
            create_function(function_name)
        except ClientError as e:
            if e.response['Error']['Code'] == 'ResourceConflictException':
                print('Skipping function creation since it already exists...')
            else:
                print('Unexpected error: {}'.format(e))
                raise
        print('Creation successful!')

    if args.invoke or args.invoke_local:
        log_function = not args.quiet and not args.quieter
        log_control = not args.quieter
        host = args.host
        log_port = args.port
        control_port = log_port + 1
        if args.num_ops == -1:
            args.num_ops = num_ops(args.system, args.obj_size)
        if args.mode.startswith('scale'):
            _, mode, workers_per_trigger, trigger_period, num_triggers = args.mode.split(':')
            workers_per_trigger = int(workers_per_trigger)
            trigger_period = int(trigger_period)
            num_triggers = int(num_triggers)
            num_functions = workers_per_trigger * num_triggers
            lp = log_process(host, log_port, num_functions, log_function)
            op = control_process(host, control_port, workers_per_trigger, num_triggers, trigger_period, log_control)
            processes = invoke_n(args, mode, num_functions)
            processes.append(lp)
            processes.append(op)
        else:
            lp = log_process(host, log_port)
            op = control_process(host, control_port)
            processes = [invoke(args, args.mode, 1), lp, op]

        for p in processes:
            if p is not None:
                p.join()
                print('... {} terminated ...'.format(p))


if __name__ == '__main__':
    main()
