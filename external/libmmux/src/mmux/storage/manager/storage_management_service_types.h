/**
 * Autogenerated by Thrift Compiler (0.11.0)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */
#ifndef storage_management_service_TYPES_H
#define storage_management_service_TYPES_H

#include <iosfwd>

#include <thrift/Thrift.h>
#include <thrift/TApplicationException.h>
#include <thrift/TBase.h>
#include <thrift/protocol/TProtocol.h>
#include <thrift/transport/TTransport.h>

#include <thrift/stdcxx.h>


namespace mmux { namespace storage {

enum rpc_block_state {
  rpc_regular = 0,
  rpc_importing = 1,
  rpc_exporting = 2
};

extern const std::map<int, const char*> _rpc_block_state_VALUES_TO_NAMES;

std::ostream& operator<<(std::ostream& out, const rpc_block_state val);

class storage_management_exception;

class rpc_slot_range;

typedef struct _storage_management_exception__isset {
  _storage_management_exception__isset() : msg(false) {}
  bool msg :1;
} _storage_management_exception__isset;

class storage_management_exception : public ::apache::thrift::TException {
 public:

  storage_management_exception(const storage_management_exception&);
  storage_management_exception& operator=(const storage_management_exception&);
  storage_management_exception() : msg() {
  }

  virtual ~storage_management_exception() throw();
  std::string msg;

  _storage_management_exception__isset __isset;

  void __set_msg(const std::string& val);

  bool operator == (const storage_management_exception & rhs) const
  {
    if (!(msg == rhs.msg))
      return false;
    return true;
  }
  bool operator != (const storage_management_exception &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const storage_management_exception & ) const;

  template <class Protocol_>
  uint32_t read(Protocol_* iprot);
  template <class Protocol_>
  uint32_t write(Protocol_* oprot) const;

  virtual void printTo(std::ostream& out) const;
  mutable std::string thriftTExceptionMessageHolder_;
  const char* what() const throw();
};

void swap(storage_management_exception &a, storage_management_exception &b);

std::ostream& operator<<(std::ostream& out, const storage_management_exception& obj);


class rpc_slot_range {
 public:

  rpc_slot_range(const rpc_slot_range&);
  rpc_slot_range& operator=(const rpc_slot_range&);
  rpc_slot_range() : slot_begin(0), slot_end(0) {
  }

  virtual ~rpc_slot_range() throw();
  int32_t slot_begin;
  int32_t slot_end;

  void __set_slot_begin(const int32_t val);

  void __set_slot_end(const int32_t val);

  bool operator == (const rpc_slot_range & rhs) const
  {
    if (!(slot_begin == rhs.slot_begin))
      return false;
    if (!(slot_end == rhs.slot_end))
      return false;
    return true;
  }
  bool operator != (const rpc_slot_range &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const rpc_slot_range & ) const;

  template <class Protocol_>
  uint32_t read(Protocol_* iprot);
  template <class Protocol_>
  uint32_t write(Protocol_* oprot) const;

  virtual void printTo(std::ostream& out) const;
};

void swap(rpc_slot_range &a, rpc_slot_range &b);

std::ostream& operator<<(std::ostream& out, const rpc_slot_range& obj);

}} // namespace

#include "storage_management_service_types.tcc"

#endif
