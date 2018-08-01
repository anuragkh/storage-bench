/**
 * Autogenerated by Thrift Compiler (0.11.0)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */
#ifndef subscription_service_TYPES_H
#define subscription_service_TYPES_H

#include <iosfwd>

#include <thrift/Thrift.h>
#include <thrift/TApplicationException.h>
#include <thrift/TBase.h>
#include <thrift/protocol/TProtocol.h>
#include <thrift/transport/TTransport.h>

#include <thrift/stdcxx.h>


namespace mmux { namespace storage {

enum response_type {
  subscribe = 0,
  unsubscribe = 1
};

extern const std::map<int, const char*> _response_type_VALUES_TO_NAMES;

std::ostream& operator<<(std::ostream& out, const response_type val);

}} // namespace

#include "subscription_service_types.tcc"

#endif
