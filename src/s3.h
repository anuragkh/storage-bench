#ifndef STORAGE_BENCH_S_3_H
#define STORAGE_BENCH_S_3_H

#include <queue>
#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/core/utils/stream/SimpleStreamBuf.h>
#include "storage_interface.h"
#include "queue.h"

class s3 : public storage_interface {
 public:
  static const int TIMEOUT_MAX = 20;
  s3();
  ~s3();

  void init(const property_map &conf, bool create) override;
  void write(const std::string &key, const std::string &value) override;
  std::string read(const std::string &key) override;
  void destroy() override;
  void write_async(const std::string &key, const std::string &value) override;
  void read_async(const std::string &key) override;
  void wait_write() override;
  std::string wait_read() override;

 private:
  void empty_bucket(const Aws::String &bucket_name);
  void wait_for_bucket_to_empty(const Aws::String &bucket_name);
  void delete_bucket(const Aws::String &bucket_name);
  bool wait_for_bucket_to_propagate();

  Aws::S3::Model::PutObjectRequest make_put_request(const std::string &key, const std::string &value) const;
  Aws::S3::Model::GetObjectRequest make_get_request(const std::string &key) const;
  std::string parse_get_response(Aws::S3::Model::GetObjectOutcome &outcome) const;
  void parse_put_response(Aws::S3::Model::PutObjectOutcome &outcome) const;

  queue<Aws::S3::Model::PutObjectOutcomeCallable> m_put_callables;
  queue<Aws::S3::Model::GetObjectOutcomeCallable> m_get_callables;
  Aws::String m_bucket_name;
  std::shared_ptr<Aws::S3::S3Client> m_client;
};

#endif //STORAGE_BENCH_S_3_H
