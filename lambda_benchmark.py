from __future__ import print_function

import argparse
import json
import multiprocessing
import os

import boto3
from botocore.exceptions import ClientError
from six.moves import configparser

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


def parse_ini(conf_file, system):
    config = configparser.ConfigParser()
    config.read(conf_file)
    return dict(config.items(system))


def invoke_function(name, system, conf_file):
    event = dict(system=system, conf=parse_ini(system, conf_file))
    lambda_client.invoke(
        FunctionName=name,
        InvocationType='Event',
        Payload=json.dumps(event),
    )


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Run storage benchmark on AWS Lambda.')
    parser.add_argument('--invoke', action='store_true', help='invoke AWS Lambda function')
    parser.add_argument('--system', type=str, default='s3', help='system to benchmark')
    parser.add_argument('--conf', type=str, default='conf/storage_bench.conf', help='configuration file')
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

    if args.invoke:
        invoke_function(function_name, args.system, args.conf)
