#include "dynamodb.h"

#include <aws/core/utils/Outcome.h>
#include <aws/dynamodb/model/CreateTableRequest.h>
#include <aws/dynamodb/model/DeleteTableRequest.h>
#include <aws/dynamodb/model/DescribeTableRequest.h>
#include <aws/dynamodb/model/ListTablesRequest.h>
#include <aws/dynamodb/model/UpdateTableRequest.h>
#include <aws/dynamodb/model/PutItemRequest.h>
#include <aws/dynamodb/model/GetItemRequest.h>
#include <aws/dynamodb/model/ScanRequest.h>
#include <aws/dynamodb/model/UpdateItemRequest.h>
#include <aws/dynamodb/model/DeleteItemRequest.h>

using namespace Aws::Auth;
using namespace Aws::Http;
using namespace Aws::Client;
using namespace Aws::DynamoDB;
using namespace Aws::DynamoDB::Model;

dynamodb::dynamodb() = default;

dynamodb::~dynamodb() = default;

void dynamodb::init(const property_map &conf, bool create) {
  // Create a client
  ClientConfiguration config;
  m_client = Aws::MakeShared<DynamoDBClient>("DynamoDBBenchmark", config);

  // Set table

  std::string table_name = conf.get<std::string>("table_name", "test");
  if (table_name == "test") {
    table_name += (std::string(".") + random_string(10));
    create = true;
  }
  m_table_name = Aws::String(table_name.data());

  if (create) {
    create_table(conf.get<long long>("read_capacity", 10000), conf.get<long long>("write_capacity", 10000));
  }
  wait_for_table();
}

void dynamodb::write(const std::string &key, const std::string &value) {
  parse_put_response(m_client->PutItem(make_put_request(key, value)));
}

std::string dynamodb::read(const std::string &key) {
  return parse_get_response(m_client->GetItem(make_get_request(key)));
}

void dynamodb::destroy() {
  DeleteTableRequest request;
  request.SetTableName(m_table_name);

  auto outcome = m_client->DeleteTable(request);

  if (!outcome.IsSuccess()) {
    // It's okay if the table has already been deleted
    if (outcome.GetError().GetErrorType() == DynamoDBErrors::RESOURCE_NOT_FOUND) {
      std::cerr << "Could not delete table " << m_table_name << ": table not found" << std::endl;
    } else {
      std::cerr << "Error deleting table " << m_table_name << ": " << outcome.GetError().GetMessage() << std::endl;
      exit(1);
    }
    return;
  }
}

void dynamodb::write_async(const std::string &key, const std::string &value) {
  m_put_callables.push(m_client->PutItemCallable(make_put_request(key, value)));
}

void dynamodb::read_async(const std::string &key) {
  m_get_callables.push(m_client->GetItemCallable(make_get_request(key)));
}

void dynamodb::wait_write() {
  parse_put_response(m_put_callables.pop().get());
}

std::string dynamodb::wait_read() {
  return parse_get_response(m_get_callables.pop().get());
}

PutItemRequest dynamodb::make_put_request(const std::string &key, const std::string &value) const {
  PutItemRequest request;
  request.SetTableName(m_table_name);
  AttributeValue key_attr;
  key_attr.SetS(key.c_str());
  request.AddItem(HASH_KEY_NAME, key_attr);
  AttributeValue value_attr;
  value_attr.SetS(value.c_str());
  request.AddItem(VALUE_NAME, value_attr);
  return request;
}

GetItemRequest dynamodb::make_get_request(const std::string &key) const {
  GetItemRequest request;
  AttributeValue hashKey;
  hashKey.SetS(key.c_str());
  request.AddKey(HASH_KEY_NAME, hashKey);
  request.SetTableName(m_table_name);
  Aws::Vector<Aws::String> attributesToGet;
  attributesToGet.push_back(HASH_KEY_NAME);
  attributesToGet.push_back(VALUE_NAME);
  request.SetAttributesToGet(attributesToGet);
  return request;
}

void dynamodb::parse_put_response(const Aws::DynamoDB::Model::PutItemOutcome &outcome) const {
  if (!outcome.IsSuccess()) {
    throw std::runtime_error(outcome.GetError().GetMessage().c_str());
  }
}

std::string dynamodb::parse_get_response(const GetItemOutcome &outcome) const {
  if (!outcome.IsSuccess()) {
    throw std::runtime_error(outcome.GetError().GetMessage().c_str());
  }
  return std::string(outcome.GetResult().GetItem().at(HASH_KEY_NAME).GetS().data());
}

void dynamodb::wait_for_table() {
  DescribeTableRequest request;
  request.SetTableName(m_table_name);
  DescribeTableOutcome outcome = m_client->DescribeTable(request);

  while (true) {
    if (!outcome.IsSuccess()) {
      std::cerr << "Error waiting for table " << m_table_name << ":" << outcome.GetError().GetMessage() << std::endl;
      exit(1);
    }
    if (outcome.GetResult().GetTable().GetTableStatus() == TableStatus::ACTIVE) {
      break;
    } else {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    outcome = m_client->DescribeTable(request);
  }
}

void dynamodb::create_table(long long read_capacity, long long write_capacity) {
  CreateTableRequest request;
  AttributeDefinition hashKey;
  hashKey.SetAttributeName(HASH_KEY_NAME);
  hashKey.SetAttributeType(ScalarAttributeType::S);
  request.AddAttributeDefinitions(hashKey);
  KeySchemaElement hashKeySchemaElement;
  hashKeySchemaElement.WithAttributeName(HASH_KEY_NAME).WithKeyType(KeyType::HASH);
  request.AddKeySchema(hashKeySchemaElement);

  ProvisionedThroughput t;
  t.SetReadCapacityUnits(read_capacity);
  t.SetWriteCapacityUnits(write_capacity);
  request.WithProvisionedThroughput(t);
  request.WithTableName(m_table_name);

  CreateTableOutcome outcome = m_client->CreateTable(request);
  if (!outcome.IsSuccess()) {
    if (outcome.GetError().GetExceptionName() != "ResourceInUseException") {
      std::cerr << "Error creating table " << m_table_name << ": " << outcome.GetError().GetExceptionName()
                << std::endl;
      exit(1);
    } else {
      std::cerr << "Table " << m_table_name << " already exists" << std::endl;
    }
  }
}

REGISTER_STORAGE_IFACE("dynamodb", dynamodb);
