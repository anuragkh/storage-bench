#ifndef STORAGE_BENCH_DYNAMODB_H
#define STORAGE_BENCH_DYNAMODB_H

#include "storage_interface.h"

#include <aws/dynamodb/DynamoDBClient.h>
#include <aws/core/Aws.h>

class dynamodb: public storage_interface {
 public:
  static constexpr const char *HASH_KEY_NAME = "HashKey";
  static constexpr const char *VALUE_NAME = "Value";

  void init(const property_map &conf) override;
  void write(const std::string &key, const std::string &value) override;
  std::string read(const std::string &key) override;
  void destroy() override;

 private:
  Aws::String m_table_name;
  std::shared_ptr<Aws::DynamoDB::DynamoDBClient> m_client;
  Aws::SDKOptions m_options;
};

#endif //STORAGE_BENCH_DYNAMODB_H
