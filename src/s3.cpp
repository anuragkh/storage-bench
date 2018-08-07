#include "s3.h"

#include <aws/s3/model/DeleteBucketRequest.h>
#include <aws/s3/model/CreateBucketRequest.h>
#include <aws/s3/model/HeadBucketRequest.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/DeleteObjectRequest.h>
#include <aws/s3/model/ListObjectsRequest.h>
#include <aws/core/utils/stream/SimpleStreamBuf.h>
#include <aws/core/utils/logging/DefaultLogSystem.h>
#include <aws/core/utils/logging/AWSLogging.h>
#include <fstream>

using namespace Aws::Auth;
using namespace Aws::Http;
using namespace Aws::Client;
using namespace Aws::S3;
using namespace Aws::S3::Model;

void s3::init(const storage_interface::property_map &conf) {
  m_options.loggingOptions.logLevel = Aws::Utils::Logging::LogLevel::Warn;
  Aws::InitAPI(m_options);

  // Create a client
  ClientConfiguration config;
  m_client = Aws::MakeShared<S3Client>("S3Benchmark", config);

  // Create the bucket
  auto bucket_name = conf.get<std::string>("bucket_name", "test") + "." + random_string(10);
  m_bucket_name = Aws::String(bucket_name.data());

  CreateBucketRequest createBucketRequest;
  createBucketRequest.SetBucket(m_bucket_name);
  createBucketRequest.SetACL(BucketCannedACL::private_);

  auto outcome = m_client->CreateBucket(createBucketRequest);
  if (!outcome.IsSuccess()) {
    std::cerr << "Failed to create bucket " << bucket_name << ": " << outcome.GetError().GetMessage() << std::endl;
    exit(-1);
  }
  wait_for_bucket_to_propagate();
}

void s3::write(const std::string &key, const std::string &value) {
  Aws::S3::Model::PutObjectRequest object_request;
  object_request.WithBucket(m_bucket_name).WithKey(key.c_str());

  Aws::Utils::Stream::SimpleStreamBuf sbuf;
  auto out = Aws::MakeShared<Aws::IOStream>("StreamBuf", &sbuf);
  out->write(value.c_str(), value.length());
  out->seekg(0, std::ios_base::beg);
  object_request.SetBody(out);
  auto put_object_outcome = m_client->PutObject(object_request);
  if (!put_object_outcome.IsSuccess()) {
    throw std::runtime_error(put_object_outcome.GetError().GetMessage().c_str());
  }
}

std::string s3::read(const std::string &key) {
  Aws::S3::Model::GetObjectRequest object_request;
  object_request.WithBucket(m_bucket_name).WithKey(key.c_str());

  auto get_object_outcome = m_client->GetObject(object_request);
  if (!get_object_outcome.IsSuccess()) {
    throw std::runtime_error(get_object_outcome.GetError().GetMessage().c_str());
  }
  auto in = std::make_shared<Aws::IOStream>(get_object_outcome.GetResult().GetBody().rdbuf());
  return std::string(std::istreambuf_iterator<char>(*in), std::istreambuf_iterator<char>());
}

void s3::destroy() {
  HeadBucketRequest headBucketRequest;
  headBucketRequest.SetBucket(m_bucket_name);
  HeadBucketOutcome bucketOutcome = m_client->HeadBucket(headBucketRequest);

  if (bucketOutcome.IsSuccess()) {
    empty_bucket();

    DeleteBucketRequest deleteBucketRequest;
    deleteBucketRequest.SetBucket(m_bucket_name);

    DeleteBucketOutcome deleteBucketOutcome = m_client->DeleteBucket(deleteBucketRequest);
    if (!deleteBucketOutcome.IsSuccess()) {
      std::cerr << "Failed to delete bucket " << m_bucket_name << ": " << deleteBucketOutcome.GetError().GetMessage()
                << std::endl;
      exit(-1);
    }
  }

  Aws::ShutdownAPI(m_options);
}

void s3::empty_bucket() {
  ListObjectsRequest listObjectsRequest;
  listObjectsRequest.SetBucket(m_bucket_name);

  ListObjectsOutcome listObjectsOutcome = m_client->ListObjects(listObjectsRequest);

  if (!listObjectsOutcome.IsSuccess())
    return;

  for (const auto &object : listObjectsOutcome.GetResult().GetContents()) {
    DeleteObjectRequest deleteObjectRequest;
    deleteObjectRequest.SetBucket(m_bucket_name);
    deleteObjectRequest.SetKey(object.GetKey());
    m_client->DeleteObject(deleteObjectRequest);
  }

  wait_for_bucket_to_empty();
}

void s3::wait_for_bucket_to_empty() {
  ListObjectsRequest listObjectsRequest;
  listObjectsRequest.SetBucket(m_bucket_name);

  unsigned checkForObjectsCount = 0;
  while (checkForObjectsCount++ < TIMEOUT_MAX) {
    ListObjectsOutcome listObjectsOutcome = m_client->ListObjects(listObjectsRequest);
    if (!listObjectsOutcome.IsSuccess()) {
      std::cerr << "Failed to list objects on bucket " << m_bucket_name << ": "
                << listObjectsOutcome.GetError().GetMessage() << std::endl;
      std::exit(-1);
    }

    if (!listObjectsOutcome.GetResult().GetContents().empty()) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    } else {
      break;
    }
  }
}

bool s3::wait_for_bucket_to_propagate() {
  unsigned timeoutCount = 0;
  while (timeoutCount++ < TIMEOUT_MAX)
  {
    HeadBucketRequest headBucketRequest;
    headBucketRequest.SetBucket(m_bucket_name);
    HeadBucketOutcome headBucketOutcome = m_client->HeadBucket(headBucketRequest);
    if (headBucketOutcome.IsSuccess())
    {
      return true;
    }

    std::this_thread::sleep_for(std::chrono::seconds(10));
  }

  return false;
}

REGISTER_STORAGE_IFACE("s3", s3);
