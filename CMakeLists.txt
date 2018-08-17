cmake_minimum_required(VERSION 3.9)
project(storage-bench C CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(PROJECT_NAME "storage-bench")
set(PROJECT_VERSION_MAJOR "0")
set(PROJECT_VERSION_MINOR "1")
set(PROJECT_VERSION_PATCH "0")
set(PROJECT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake-modules")

set(CMAKE_CXX_FLAGS "-fPIC ${CMAKE_CXX_FLAGS}")
set(CMAKE_C_FLAGS "-fPIC ${CMAKE_C_FLAGS}")
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
        src/benchmark.h
        src/dynamodb.cpp
        src/dynamodb.h
        src/s3.cpp
        src/s3.h
        src/redis.cpp
        src/redis.h src/memorymux.cpp src/memorymux.h src/key_generator.h src/key_generator.cpp)

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

set(ZIP_FILE ${CMAKE_BINARY_DIR}/lambda.zip)
set(BENCHMARK_RUNNER_SRC "${CMAKE_SOURCE_DIR}/src/benchmark_handler.py")
set(BENCHMARK_RUNNER "${CMAKE_BINARY_DIR}/benchmark_handler.py")
set(STORAGE_BENCH "${CMAKE_BINARY_DIR}/storage_bench")
set(FILES_TO_ZIP ${CMAKE_SOURCE_DIR}/src/benchmark_handler.py ${CMAKE_BINARY_DIR}/storage_bench)
message(STATUS "Files to zip: ${FILES_TO_ZIP}")

add_custom_command(OUTPUT ${ZIP_FILE}
        COMMAND ${CMAKE_COMMAND} -E copy ${BENCHMARK_RUNNER_SRC} ${BENCHMARK_RUNNER}
        COMMAND ${CMAKE_COMMAND} -E tar cfv ${ZIP_FILE} --format=zip -- ${STORAGE_BENCH} ${BENCHMARK_RUNNER}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        DEPENDS storage_bench
        COMMENT "Packaging code"
        VERBATIM)
add_custom_target(pkg DEPENDS ${ZIP_FILE})