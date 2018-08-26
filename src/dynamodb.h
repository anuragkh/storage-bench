#ifndef STORAGE_BENCH_DYNAMODB_H
#define STORAGE_BENCH_DYNAMODB_H

#include "storage_interface.h"

#include <aws/dynamodb/DynamoDBClient.h>
#include <aws/core/Aws.h>
#include "queue.h"

class dynamodb: public storage_interface {
 public:
  static constexpr const char *HASH_KEY_NAME = "HashKey";
  static constexpr const char *VALUE_NAME = "Value";

  dynamodb();
  ~dynamodb();

  void init(const property_map &conf, bool create) override;
  void write(const std::string &key, const std::string &value) override;
  std::string read(const std::string &key) override;
  void destroy() override;
  void write_async(const std::string &key, const std::string &value) override;
  void read_async(const std::string &key) override;
  void wait_write() override;
  std::string wait_read() override;

 private:
  void create_table(long long read_capacity, long long write_capacity);
  void wait_for_table();

  Aws::DynamoDB::Model::PutItemRequest make_put_request(const std::string &key, const std::string &value) const;
  Aws::DynamoDB::Model::GetItemRequest make_get_request(const std::string &key) const;
  void parse_put_response(const Aws::DynamoDB::Model::PutItemOutcome& outcome) const;
  std::string parse_get_response(const Aws::DynamoDB::Model::GetItemOutcome& outcome) const;

  queue<Aws::DynamoDB::Model::PutItemOutcomeCallable> m_put_callables;
  queue<Aws::DynamoDB::Model::GetItemOutcomeCallable> m_get_callables;
  Aws::String m_table_name;
  std::shared_ptr<Aws::DynamoDB::DynamoDBClient> m_client;
};

#endif //STORAGE_BENCH_DYNAMODB_H
