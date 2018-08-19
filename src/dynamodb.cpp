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

dynamodb::dynamodb() {
  m_options.loggingOptions.logLevel = Aws::Utils::Logging::LogLevel::Warn;
  Aws::InitAPI(m_options);
}

dynamodb::~dynamodb() {
  Aws::ShutdownAPI(m_options);
}

void dynamodb::init(const property_map &conf) {
  // Create a client
  ClientConfiguration config;
  m_client = Aws::MakeShared<DynamoDBClient>("DynamoDBBenchmark", config);

  // Set table

  std::string table_name = conf.get<std::string>("table_name", "test");
  if (table_name == "test") {
    table_name += (std::string(".") + random_string(10));
  }
  m_table_name = Aws::String(table_name.data());

  {
    CreateTableRequest request;
    AttributeDefinition hashKey;
    hashKey.SetAttributeName(HASH_KEY_NAME);
    hashKey.SetAttributeType(ScalarAttributeType::S);
    request.AddAttributeDefinitions(hashKey);
    KeySchemaElement hashKeySchemaElement;
    hashKeySchemaElement.WithAttributeName(HASH_KEY_NAME).WithKeyType(KeyType::HASH);
    request.AddKeySchema(hashKeySchemaElement);

    ProvisionedThroughput t;
    t.SetReadCapacityUnits(conf.get<long long>("read_capacity", 10000));
    t.SetWriteCapacityUnits(conf.get<long long>("write_capacity", 10000));
    request.WithProvisionedThroughput(t);
    request.WithTableName(m_table_name);

    CreateTableOutcome outcome = m_client->CreateTable(request);
    if (!outcome.IsSuccess()) {
      if (outcome.GetError().GetExceptionName() != "ResourceInUseException") {
        std::cerr << "Error creating table " << m_table_name << ": " << outcome.GetError().GetExceptionName() << std::endl;
        exit(1);
      } else {
        std::cerr << "Table " << m_table_name << " already exists";
      }
    }
  }

  {
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
}

void dynamodb::write(const std::string &key, const std::string &value) {
  PutItemRequest request;
  request.SetTableName(m_table_name);
  AttributeValue key_attr;
  key_attr.SetS(key.c_str());
  request.AddItem(HASH_KEY_NAME, key_attr);
  AttributeValue value_attr;
  value_attr.SetS(value.c_str());
  request.AddItem(VALUE_NAME, value_attr);

  auto outcome = m_client->PutItem(request);
  if (!outcome.IsSuccess()) {
    throw std::runtime_error(outcome.GetError().GetMessage().c_str());
  }
}

std::string dynamodb::read(const std::string &key) {
  GetItemRequest request;
  AttributeValue hashKey;
  hashKey.SetS(key.c_str());
  request.AddKey(HASH_KEY_NAME, hashKey);
  request.SetTableName(m_table_name);

  Aws::Vector<Aws::String> attributesToGet;
  attributesToGet.push_back(HASH_KEY_NAME);
  attributesToGet.push_back(VALUE_NAME);
  auto outcome = m_client->GetItem(request);
  if (!outcome.IsSuccess()) {
    throw std::runtime_error(outcome.GetError().GetMessage().c_str());
  }
  return std::string(outcome.GetResult().GetItem().at(HASH_KEY_NAME).GetS().data());
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

REGISTER_STORAGE_IFACE("dynamodb", dynamodb);
