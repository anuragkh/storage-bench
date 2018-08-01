cmake_minimum_required(VERSION 3.9)
project(storage-bench C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(PROJECT_NAME "storage-bench")
set(PROJECT_VERSION_MAJOR "0")
set(PROJECT_VERSION_MINOR "1")
set(PROJECT_VERSION_PATCH "0")
set(PROJECT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake-modules")

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -Wall -pedantic -Wextra")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall -pedantic -Wextra")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -O3 -g -Wall -pedantic -Wextra")

set(CMAKE_BUILD_TYPE RelWithDebInfo)

include(Dependencies)

add_subdirectory(external/libmmux)

include_directories(src)
include_directories(external/libmmux/src)
add_executable(storage_bench
        src/main.cpp
        src/storage_interface.cpp
        src/storage_interface.h
        src/benchmark.cpp
        src/benchmark.h
        src/dynamodb.cpp
        src/dynamodb.h
        src/s3.cpp
        src/s3.h
        src/redis.cpp
        src/redis.h src/memorymux.cpp src/memorymux.h)

if (NOT USE_SYSTEM_BOOST)
  add_dependencies(storage_bench boost)
endif ()

if (NOT USE_SYSTEM_AWSSDK)
  add_dependencies(storage_bench awssdk)
endif ()

if (NOT USE_SYSTEM_JEMALLOC)
  add_dependencies(storage_bench jemalloc)
endif ()

if (NOT USE_SYSTEM_CPP_REDIS)
  add_dependencies(storage_bench cpp_redis)
endif ()

target_link_libraries(storage_bench
        mmux
        ${JEMALLOC_LIBRARIES}
        ${Boost_LIBRARIES}
        ${AWS_LIBRARIES}
        ${CURL_LIBRARY}
        ${OPENSSL_LIBRARIES}
        ${ZLIB_LIBRARY}
        ${CPP_REDIS_LIBRARIES})
