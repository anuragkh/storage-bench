#ifndef STORAGE_BENCH_S_3_H
#define STORAGE_BENCH_S_3_H

#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include "storage_interface.h"

class s3: public storage_interface {
 public:
  static const int TIMEOUT_MAX = 20;
  s3();
  ~s3();

  void init(const property_map &conf) override;
  void write(const std::string &key, const std::string &value) override;
  std::string read(const std::string &key) override;
  void destroy() override;

 private:
  void empty_bucket();
  void wait_for_bucket_to_empty();
  bool wait_for_bucket_to_propagate();

  Aws::String m_bucket_name;
  std::shared_ptr<Aws::S3::S3Client> m_client;
  Aws::SDKOptions m_options;
};

#endif //STORAGE_BENCH_S_3_H
