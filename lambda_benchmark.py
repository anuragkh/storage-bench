from __future__ import print_function

import argparse
import errno
import json
import multiprocessing
import os
import socket
import sys
import time
from multiprocessing import Process

import boto3
from botocore.exceptions import ClientError
from six.moves import configparser

from src import benchmark_handler

iam_client = boto3.client('iam')
lambda_client = boto3.client('lambda')


def create_role():
    role_policy_document = {
        "Version": "2012-10-17",
        "Statement": [
            {
                "Sid": "",
                "Effect": "Allow",
                "Principal": {
                    "Service": "lambda.amazonaws.com"
                },
                "Action": [
                    "sts:AssumeRole",
                ],
            },
            {
                "Effect": "Allow",
                "Action": [
                    "logs:CreateLogGroup",
                    "logs:CreateLogStream",
                    "logs:PutLogEvents",
                    "logs:DescribeLogStreams"
                ],
                "Resource": [
                    "arn:aws:logs:*:*:*"
                ]
            }
        ]
    }
    iam_client.create_role(
        RoleName='LambdaBasicExecution',
        AssumeRolePolicyDocument=json.dumps(role_policy_document),
    )


def create_function(name):
    env = dict()
    lambda_zip = '/tmp/build/lambda.zip'
    if not os.path.isfile(lambda_zip):
        code_path = os.path.dirname(os.path.realpath(__file__))
        num_cpu = multiprocessing.cpu_count()
        os.system('mkdir -p /tmp/build && cd /tmp/build && cmake {} && make -j {} pkg'.format(code_path, num_cpu))
    with open(lambda_zip, 'rb') as f:
        zipped_code = f.read()
    role = iam_client.get_role(RoleName='LambdaBasicExecution')
    lambda_client.create_function(
        FunctionName=name,
        Runtime='python3.6',
        Role=role['Role']['Arn'],
        Handler='benchmark_handler.benchmark_handler',
        Code=dict(ZipFile=zipped_code),
        Timeout=300,
        Environment=dict(Variables=env),
    )


def parse_ini(system, conf_file):
    config = configparser.ConfigParser()
    config.read(conf_file)
    return dict(config.items(system))


def invoke(name, system, conf_file, host, port, bin_path, object_sizes):
    event = dict(system=system, conf=parse_ini(system, conf_file), host=host, port=port, bin_path=bin_path,
                 object_sizes=object_sizes)
    lambda_client.invoke(
        FunctionName=name,
        InvocationType='Event',
        Payload=json.dumps(event),
    )


def invoke_locally(system, conf_file, host, port, bin_path, object_sizes):
    event = dict(system=system, conf=parse_ini(system, conf_file), host=host, port=port, bin_path=bin_path,
                 object_sizes=object_sizes)
    function_process = Process(target=benchmark_handler.benchmark_handler, args=(event, None,))
    function_process.start()
    return function_process


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
    try:
        s.bind((host, port))
    except socket.error as ex:
        print('Bind failed: {}'.format(ex))
        sys.exit()
    s.listen(0)
    print('Listening for function logs on {}:{}'.format(host, port))
    sock, address = s.accept()
    print('Received connection from {}'.format(address))
    while is_socket_valid(sock):
        try:
            data = sock.recv(4096).rstrip().lstrip()
            if data == 'CLOSE':
                print('Function @ {} finished execution'.format(address))
                break
            elif data:
                print('FUNCTION_LOG {}'.format(data))
        except socket.error as ex:
            print("Function @ {} is offline: {}".format(address, ex))
            sock.close()
            break
    s.close()


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Run storage benchmark on AWS Lambda.')
    parser.add_argument('--create', action='store_true', help='create AWS Lambda function')
    parser.add_argument('--invoke', action='store_true', help='invoke AWS Lambda function')
    parser.add_argument('--invoke-local', action='store_true', help='invoke AWS Lambda function locally')
    parser.add_argument('--system', type=str, default='s3', help='system to benchmark')
    parser.add_argument('--conf', type=str, default='conf/storage_bench.conf', help='configuration file')
    parser.add_argument('--host', type=str, default=socket.gethostname(), help='name of host where script is run')
    parser.add_argument('--port', type=int, default=8888, help='port that server listens on')
    parser.add_argument('--bin-path', type=str, default='build', help='location of executable (local mode only)')
    parser.add_argument('--object-sizes', nargs='+', help='object sizes to benchmark for')
    args = parser.parse_args()
    function_name = "StorageBenchmark"

    if args.create:
        try:
            create_role()
        except ClientError as e:
            if e.response['Error']['Code'] == 'EntityAlreadyExists':
                print('Skipping role creation since it already exists...')
            else:
                print('Unexpected error: {}'.format(e))
                raise

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
        log_server_process = Process(target=run_server, args=(args.host, args.port))
        log_server_process.start()
        time.sleep(3)
        p = None
        if args.invoke:
            print('Invoking function...')
            invoke(function_name, args.system, args.conf, args.host, args.port, args.bin_path, args.object_sizes)
        elif args.invoke_local:
            print('Invoking function locally...')
            p = invoke_locally(args.system, args.conf, args.host, args.port, args.bin_path, args.object_sizes)

        if p is not None:
            p.join()
            print('Local function process terminated')

        log_server_process.join()
