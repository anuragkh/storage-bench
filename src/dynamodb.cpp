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

void dynamodb::init(const property_map &conf) {
  m_options.loggingOptions.logLevel = Aws::Utils::Logging::LogLevel::Warn;
  Aws::InitAPI(m_options);

  // Create a client
  ClientConfiguration config;
  m_client = Aws::MakeShared<DynamoDBClient>("DynamoDBBenchmark", config);

  CreateTableRequest createTableRequest;
  AttributeDefinition hashKey;
  hashKey.SetAttributeName(HASH_KEY_NAME);
  hashKey.SetAttributeType(ScalarAttributeType::S);
  createTableRequest.AddAttributeDefinitions(hashKey);
  KeySchemaElement hashKeySchemaElement;
  hashKeySchemaElement.WithAttributeName(HASH_KEY_NAME).WithKeyType(KeyType::HASH);
  createTableRequest.AddKeySchema(hashKeySchemaElement);
  ProvisionedThroughput provisionedThroughput;

  auto table_name = conf.get<std::string>("table_name", "test") + "." + random_string(10);
  m_table_name = Aws::String(table_name.data());

  provisionedThroughput.SetReadCapacityUnits(conf.get<long long>("read_capacity", 10000));
  provisionedThroughput.SetWriteCapacityUnits(conf.get<long long>("write_capacity", 10000));
  createTableRequest.WithProvisionedThroughput(provisionedThroughput);
  createTableRequest.WithTableName(m_table_name);

  CreateTableOutcome createTableOutcome = m_client->CreateTable(createTableRequest);
  if (createTableOutcome.IsSuccess()) {
    std::cerr << "Successfully created table " << m_table_name << std::endl;
  } else {
    std::cerr << "Error creating table " << m_table_name << ":" << createTableOutcome.GetError().GetMessage()
              << std::endl;
    exit(-1);
  }
  //since we need to wait for the table to finish creating anyways,
  //let's go ahead and test describe table api while we are at it.
  DescribeTableRequest describeTableRequest;
  describeTableRequest.SetTableName(m_table_name);
  DescribeTableOutcome outcome = m_client->DescribeTable(describeTableRequest);

  while (true) {
    if (!outcome.IsSuccess()) {
      std::cerr << "Error waiting for table " << m_table_name << ":" << outcome.GetError().GetMessage() << std::endl;
    }
    if (outcome.GetResult().GetTable().GetTableStatus() == TableStatus::ACTIVE) {
      break;
    } else {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    outcome = m_client->DescribeTable(describeTableRequest);
  }
}

void dynamodb::write(const std::string &key, const std::string &value) {
  PutItemRequest putItemRequest;
  putItemRequest.SetTableName(m_table_name);
  AttributeValue hashKeyAttribute;
  hashKeyAttribute.SetS(key.c_str());
  putItemRequest.AddItem(HASH_KEY_NAME, hashKeyAttribute);
  AttributeValue testValueAttribute;
  testValueAttribute.SetS(value.c_str());
  putItemRequest.AddItem(VALUE_NAME, testValueAttribute);

  auto outcome = m_client->PutItem(putItemRequest);
  if (!outcome.IsSuccess()) {
    throw std::runtime_error(outcome.GetError().GetMessage().c_str());
  }
}

std::string dynamodb::read(const std::string &key) {
  GetItemRequest getItemRequest;
  AttributeValue hashKey;
  hashKey.SetS(key.c_str());
  getItemRequest.AddKey(HASH_KEY_NAME, hashKey);
  getItemRequest.SetTableName(m_table_name);

  Aws::Vector<Aws::String> attributesToGet;
  attributesToGet.push_back(HASH_KEY_NAME);
  attributesToGet.push_back(VALUE_NAME);
  auto outcome = m_client->GetItem(getItemRequest);
  if (!outcome.IsSuccess()) {
    throw std::runtime_error(outcome.GetError().GetMessage().c_str());
  }
  return std::string(outcome.GetResult().GetItem().at(HASH_KEY_NAME).GetS().data());
}

void dynamodb::destroy() {
  DeleteTableRequest deleteTableRequest;
  deleteTableRequest.SetTableName(m_table_name);

  auto deleteTableOutcome = m_client->DeleteTable(deleteTableRequest);

  if (!deleteTableOutcome.IsSuccess()) {
    // It's okay if the table has already been deleted
    if (deleteTableOutcome.GetError().GetErrorType() == DynamoDBErrors::RESOURCE_NOT_FOUND) {
      std::cerr << "Could not delete table " << m_table_name << ": table not found" << std::endl;
    } else {
      std::cerr << "Error deleting table " << m_table_name << ": " << deleteTableOutcome.GetError().GetMessage()
                << std::endl;
      exit(-1);
    }
    return;
  }

  DescribeTableRequest describeTableRequest;
  describeTableRequest.SetTableName(m_table_name);
  while (true) {
    auto outcome = m_client->DescribeTable(describeTableRequest);
    if (outcome.IsSuccess() && outcome.GetResult().GetTable().GetTableStatus() == TableStatus::DELETING) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    } else if (outcome.GetError().GetErrorType() == DynamoDBErrors::RESOURCE_NOT_FOUND) {
      break;
    } else {
      std::cerr << "Unexpected control sequence" << std::endl;
      std::exit(-1);
    }
  }

  Aws::ShutdownAPI(m_options);
}

REGISTER_STORAGE_IFACE("dynamodb", dynamodb);
