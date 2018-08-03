from __future__ import print_function

import argparse
import json
import multiprocessing
import os
import select
import socket
import sys
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
                "Action": "sts:AssumeRole"
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


def invoke_function(name, system, conf_file, host, port):
    event = dict(system=system, conf=parse_ini(system, conf_file), host=host, port=port)
    lambda_client.invoke(
        FunctionName=name,
        InvocationType='Event',
        Payload=json.dumps(event),
    )


def invoke_function_locally(system, conf_file, host, port):
    event = dict(system=system, conf=parse_ini(system, conf_file), host=host, port=port)
    benchmark_handler.benchmark_handler(event, None)


def run_server(host, port):
    connections = []
    receive_buf_size = 4096

    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    try:
        s.bind((host, port))
    except socket.error as ex:
        print('Bind failed: {}'.format(ex))
        sys.exit()
    s.listen(10)
    print('Listening for function logs')

    address = None
    while True:
        read_sockets, write_sockets, error_sockets = select.select(connections, [], [])
        for sock in read_sockets:
            # New connection
            if sock == s:
                # Handle the case in which there is a new connection recieved through server_socket
                sock, address = s.accept()
                connections.append(sock)
                print("Function @ {} connected".format(address))

            # Some incoming message from a client
            else:
                # Data received from client, process it
                try:
                    # In Windows, sometimes when a TCP program closes abruptly,
                    # a "Connection reset by peer" exception will be thrown
                    data = sock.recv(receive_buf_size)
                    print(data)

                # client disconnected, so remove from socket list
                except socket.error as ex:
                    print("Function @ {} is offline: {}".format(address, ex))
                    sock.close()
                    connections.remove(sock)
                    continue


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Run storage benchmark on AWS Lambda.')
    parser.add_argument('--create', action='store_true', help='create AWS Lambda function')
    parser.add_argument('--invoke', action='store_true', help='invoke AWS Lambda function')
    parser.add_argument('--invoke-local', action='store_true', help='invoke AWS Lambda function locally')
    parser.add_argument('--system', type=str, default='s3', help='system to benchmark')
    parser.add_argument('--conf', type=str, default='conf/storage_bench.conf', help='configuration file')
    parser.add_argument('--host', type=str, default=socket.gethostname(), help='name of host where script is run')
    parser.add_argument('--port', type=int, default=8888, help='port that server listens on')
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
        p = Process(target=run_server, args=(args.host, args.port))
        p.start()
        print('Invoking function...')
        if args.invoke:
            invoke_function(function_name, args.system, args.conf, args.host, args.port)
        elif args.invoke_local:
            invoke_function_locally(args.system, args.conf, args.host, args.port)
        print('Done.')
        p.join()
