include_directories(${THRIFT_INCLUDE_DIR})
add_library(mmux STATIC
        src/mmux/client/mmux_client.cc
        src/mmux/client/mmux_client.h
        src/mmux/directory/client/directory_client.cpp
        src/mmux/directory/client/directory_client.h
        src/mmux/directory/fs/directory_service.cpp
        src/mmux/directory/fs/directory_service.h
        src/mmux/directory/fs/directory_service.tcc
        src/mmux/directory/fs/directory_service_constants.cpp
        src/mmux/directory/fs/directory_service_constants.h
        src/mmux/directory/fs/directory_service_types.cpp
        src/mmux/directory/fs/directory_service_types.h
        src/mmux/directory/fs/directory_service_types.tcc
        src/mmux/directory/fs/directory_type_conversions.h
        src/mmux/directory/client/lease_client.cpp
        src/mmux/directory/client/lease_client.h
        src/mmux/directory/client/lease_renewal_worker.cc
        src/mmux/directory/client/lease_renewal_worker.h
        src/mmux/directory/lease/lease_service.cpp
        src/mmux/directory/lease/lease_service.h
        src/mmux/directory/lease/lease_service.tcc
        src/mmux/directory/lease/lease_service_constants.cpp
        src/mmux/directory/lease/lease_service_constants.h
        src/mmux/directory/lease/lease_service_types.cpp
        src/mmux/directory/lease/lease_service_types.h
        src/mmux/directory/lease/lease_service_types.tcc
        src/mmux/directory/directory_ops.h
        src/mmux/directory/directory_ops.cpp
        src/mmux/storage/kv/serde/csv_serde.cpp
        src/mmux/storage/kv/serde/csv_serde.h
        src/mmux/storage/kv/serde/serde.h
        src/mmux/storage/kv/serde/binary_serde.cpp
        src/mmux/storage/kv/serde/binary_serde.h
        src/mmux/storage/kv/hash_slot.h
        src/mmux/storage/kv/hash_slot.cpp
        src/mmux/storage/kv/kv_block.cpp
        src/mmux/storage/kv/kv_block.h
        src/mmux/storage/kv/kv_hash.h
        src/mmux/storage/block.cc
        src/mmux/storage/block.h
        src/mmux/storage/client/replica_chain_client.cpp
        src/mmux/storage/client/replica_chain_client.h
        src/mmux/storage/client/block_client.cpp
        src/mmux/storage/client/block_client.h
        src/mmux/storage/client/block_listener.cpp
        src/mmux/storage/client/block_listener.h
        src/mmux/storage/client/kv_client.cpp
        src/mmux/storage/client/kv_client.h
        src/mmux/storage/client/kv_listener.cpp
        src/mmux/storage/client/kv_listener.h
        src/mmux/storage/service/block_request_service.cpp
        src/mmux/storage/service/block_request_service.h
        src/mmux/storage/service/block_request_service.tcc
        src/mmux/storage/service/block_response_service.cpp
        src/mmux/storage/service/block_response_service.h
        src/mmux/storage/service/block_response_service.tcc
        src/mmux/storage/service/block_service_constants.cpp
        src/mmux/storage/service/block_service_constants.h
        src/mmux/storage/service/block_service_types.cpp
        src/mmux/storage/service/block_service_types.h
        src/mmux/storage/service/block_service_types.tcc
        src/mmux/storage/manager/detail/block_name_parser.cpp
        src/mmux/storage/manager/detail/block_name_parser.h
        src/mmux/storage/notification/blocking_queue.h
        src/mmux/storage/notification/notification_handler.cpp
        src/mmux/storage/notification/notification_handler.h
        src/mmux/storage/notification/notification_service.cpp
        src/mmux/storage/notification/notification_service.h
        src/mmux/storage/notification/notification_service.tcc
        src/mmux/storage/notification/notification_service_constants.cpp
        src/mmux/storage/notification/notification_service_constants.h
        src/mmux/storage/notification/notification_service_types.cpp
        src/mmux/storage/notification/notification_service_types.h
        src/mmux/storage/notification/notification_service_types.tcc
        src/mmux/storage/notification/notification_worker.cpp
        src/mmux/storage/notification/notification_worker.h
        src/mmux/storage/notification/subscription_service.cpp
        src/mmux/storage/notification/subscription_service.h
        src/mmux/storage/notification/subscription_service.tcc
        src/mmux/storage/notification/subscription_service_constants.cpp
        src/mmux/storage/notification/subscription_service_constants.h
        src/mmux/storage/notification/subscription_service_types.cpp
        src/mmux/storage/notification/subscription_service_types.h
        src/mmux/storage/notification/subscription_service_types.tcc
        src/mmux/utils/client_cache.h
        src/mmux/utils/logger.h
        src/mmux/utils/logger.cpp
        src/mmux/utils/time_utils.h
        src/mmux/utils/string_utils.h)

if (NOT USE_SYSTEM_LIBEVENT)
  add_dependencies(mmux libevent)
endif ()

if (NOT USE_SYSTEM_THRIFT)
  add_dependencies(mmux thrift)
endif ()

if (NOT USE_SYSTEM_LIBCUCKOO)
  add_dependencies(mmux libcuckoo)
endif ()

target_link_libraries(mmux ${THRIFT_LIBRARIES}
        ${THRIFTNB_LIBRARIES}
        ${LIBEVENT_LIBRARIES}
        ${OPENSSL_LIBRARIES}
        ${ZLIB_LIBRARY}
        ${CMAKE_DL_LIBS}
        ${CMAKE_THREAD_LIBS_INIT})
set_target_properties(mmux PROPERTIES LINKER_LANGUAGE CXX)

install(TARGETS mmux
        RUNTIME DESTINATION bin
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib)
install(DIRECTORY src/mmux
        DESTINATION include
        FILES_MATCHING PATTERN "*.h")