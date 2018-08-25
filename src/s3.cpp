#include "s3.h"

#include <aws/s3/model/DeleteBucketRequest.h>
#include <aws/s3/model/CreateBucketRequest.h>
#include <aws/s3/model/HeadBucketRequest.h>
#include <aws/s3/model/DeleteObjectRequest.h>
#include <aws/s3/model/ListObjectsRequest.h>
#include <aws/core/utils/stream/SimpleStreamBuf.h>

using namespace Aws::Auth;
using namespace Aws::Http;
using namespace Aws::Client;
using namespace Aws::S3;
using namespace Aws::S3::Model;

s3::s3() {
  m_options.loggingOptions.logLevel = Aws::Utils::Logging::LogLevel::Warn;
  Aws::InitAPI(m_options);
}

s3::~s3() {
  Aws::ShutdownAPI(m_options);
}

void s3::init(const property_map &conf, bool create) {
  // Create a client
  ClientConfiguration config;
  m_client = Aws::MakeShared<S3Client>("S3Benchmark", config);

  // Create the bucket
  auto bucket_name = conf.get<std::string>("bucket_name", "test");
  if (bucket_name == "test") {
    bucket_name += (std::string(".") + random_string(10));
    create = true;
  }
  m_bucket_name = Aws::String(bucket_name.data());

  CreateBucketRequest request;
  request.SetBucket(m_bucket_name);
  request.SetACL(BucketCannedACL::private_);

  if (create) {
    auto outcome = m_client->CreateBucket(request);
    if (!outcome.IsSuccess()) {
      auto exception_name = outcome.GetError().GetExceptionName();
      std::cerr << "Failed to create bucket " << bucket_name << ": [" << exception_name << "]" << std::endl;
      exit(1);
    }
    wait_for_bucket_to_propagate();
  }
}

void s3::write(const std::string &key, const std::string &value) {
  auto outcome = m_client->PutObject(make_put_request(key, value));
  parse_put_response(outcome);
}

std::string s3::read(const std::string &key) {
  auto outcome = m_client->GetObject(make_get_request(key));
  return parse_get_response(outcome);
}

void s3::destroy() {
  HeadBucketRequest request1;
  request1.SetBucket(m_bucket_name);
  HeadBucketOutcome outcome1 = m_client->HeadBucket(request1);

  if (outcome1.IsSuccess()) {
    empty_bucket();

    DeleteBucketRequest request2;
    request2.SetBucket(m_bucket_name);

    DeleteBucketOutcome outcome2 = m_client->DeleteBucket(request2);
    if (!outcome2.IsSuccess()) {
      std::cerr << "Failed to delete bucket " << m_bucket_name << ": " << outcome2.GetError().GetMessage() << std::endl;
      exit(1);
    }
  }
}

void s3::empty_bucket() {
  ListObjectsRequest request1;
  request1.SetBucket(m_bucket_name);

  ListObjectsOutcome outcome1 = m_client->ListObjects(request1);

  if (!outcome1.IsSuccess())
    return;

  for (const auto &object : outcome1.GetResult().GetContents()) {
    DeleteObjectRequest request2;
    request2.SetBucket(m_bucket_name);
    request2.SetKey(object.GetKey());
    m_client->DeleteObject(request2);
  }

  wait_for_bucket_to_empty();
}

void s3::wait_for_bucket_to_empty() {
  ListObjectsRequest request;
  request.SetBucket(m_bucket_name);

  unsigned checkForObjectsCount = 0;
  while (checkForObjectsCount++ < TIMEOUT_MAX) {
    ListObjectsOutcome outcome = m_client->ListObjects(request);
    if (!outcome.IsSuccess()) {
      std::cerr << "Failed to list objects on bucket " << m_bucket_name << ": " << outcome.GetError().GetMessage()
                << std::endl;
      std::exit(1);
    }

    if (!outcome.GetResult().GetContents().empty()) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    } else {
      break;
    }
  }
}

bool s3::wait_for_bucket_to_propagate() {
  unsigned timeoutCount = 0;
  while (timeoutCount++ < TIMEOUT_MAX) {
    HeadBucketRequest request;
    request.SetBucket(m_bucket_name);
    HeadBucketOutcome outcome = m_client->HeadBucket(request);
    if (outcome.IsSuccess()) {
      return true;
    }

    std::this_thread::sleep_for(std::chrono::seconds(10));
  }

  return false;
}

void s3::write_async(const std::string &key, const std::string &value) {
  m_put_callables.push(m_client->PutObjectCallable(make_put_request(key, value)));
}

void s3::read_async(const std::string &key) {
  m_get_callables.push(m_client->GetObjectCallable(make_get_request(key)));
}

void s3::wait_write() {
  auto outcome = m_put_callables.pop().get();
  parse_put_response(outcome);
}

std::string s3::wait_read() {
  auto outcome = m_get_callables.pop().get();
  return parse_get_response(outcome);
}

Aws::S3::Model::PutObjectRequest s3::make_put_request(const std::string &key, const std::string &value) const {
  Aws::S3::Model::PutObjectRequest request;
  request.WithBucket(m_bucket_name).WithKey(key.c_str());

  Aws::Utils::Stream::SimpleStreamBuf sbuf;
  auto out = Aws::MakeShared<Aws::IOStream>("StreamBuf", &sbuf);
  out->write(value.c_str(), value.length());
  out->seekg(0, std::ios_base::beg);
  request.SetBody(out);
  return request;
}

Aws::S3::Model::GetObjectRequest s3::make_get_request(const std::string &key) const {
  Aws::S3::Model::GetObjectRequest request;
  request.WithBucket(m_bucket_name).WithKey(key.c_str());
  return request;
}

std::string s3::parse_get_response(Aws::S3::Model::GetObjectOutcome &outcome) const {
  if (!outcome.IsSuccess())
    throw std::runtime_error(outcome.GetError().GetMessage().c_str());
  auto in = std::make_shared<Aws::IOStream>(outcome.GetResult().GetBody().rdbuf());
  return std::string(std::istreambuf_iterator<char>(*in), std::istreambuf_iterator<char>());
}

void s3::parse_put_response(Aws::S3::Model::PutObjectOutcome &outcome) const {
  if (!outcome.IsSuccess())
    throw std::runtime_error(outcome.GetError().GetMessage().c_str());
}

REGISTER_STORAGE_IFACE("s3", s3);
