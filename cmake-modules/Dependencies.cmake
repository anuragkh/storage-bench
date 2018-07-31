include(ExternalProject)

set(ZLIB_VERSION "1.2.11")
set(OPENSSL_VERSION "1.1.1-pre7")
set(CURL_VERSION "7.60.0")
set(AWSSDK_VERSION "1.4.26")
set(BOOST_VERSION "1.63.0")
set(CPP_REDIS_VERSION "4.3.1")

set(BOOST_COMPONENTS "program_options")

## Prefer static to dynamic libraries
set(CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_FIND_LIBRARY_SUFFIXES})

find_package(Threads REQUIRED)

set(EXTERNAL_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC ${CMAKE_CXX_FLAGS_${UPPERCASE_BUILD_TYPE}}")
set(EXTERNAL_C_FLAGS "${CMAKE_C_FLAGS} -fPIC ${CMAKE_C_FLAGS_${UPPERCASE_BUILD_TYPE}}")

if (USE_SYSTEM_BOOST)
  find_package(Boost ${BOOST_VERSION} COMPONENTS program_options REQUIRED)
else ()
  foreach (component ${BOOST_COMPONENTS})
    list(APPEND BOOST_COMPONENTS_FOR_BUILD --with-${component})
  endforeach ()

  string(REGEX REPLACE "\\." "_" BOOST_VERSION_STR ${BOOST_VERSION})
  set(BOOST_CXX_FLAGS "${EXTERNAL_CXX_FLAGS}")
  set(BOOST_C_FLAGS "${EXTERNAL_C_FLAGS}")
  set(BOOST_PREFIX "${PROJECT_BINARY_DIR}/external/boost")
  set(BOOST_INCLUDE_DIR "${BOOST_PREFIX}/include")
  ExternalProject_Add(boost
          URL https://downloads.sourceforge.net/project/boost/boost/${BOOST_VERSION}/boost_${BOOST_VERSION_STR}.zip
          UPDATE_COMMAND ""
          CONFIGURE_COMMAND ./bootstrap.sh --prefix=${BOOST_PREFIX}
          BUILD_COMMAND ./b2 link=static --prefix=${BOOST_PREFIX} ${BOOST_COMPONENTS_FOR_BUILD} install
          BUILD_IN_SOURCE true
          INSTALL_COMMAND ""
          INSTALL_DIR ${BOOST_PREFIX}
          LOG_DOWNLOAD ON
          LOG_CONFIGURE ON
          LOG_BUILD ON
          LOG_INSTALL ON)

  macro(libraries_to_fullpath out)
    set(${out})
    foreach (comp ${BOOST_COMPONENTS})
      list(APPEND ${out} ${BOOST_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}boost_${comp}${CMAKE_STATIC_LIBRARY_SUFFIX})
    endforeach ()
  endmacro()
  libraries_to_fullpath(BOOST_LIBRARIES)
  set(Boost_INCLUDE_DIRS ${BOOST_INCLUDE_DIR})
  set(Boost_LIBRARIES ${BOOST_LIBRARIES})

  include_directories(SYSTEM ${Boost_INCLUDE_DIRS})
  message(STATUS "Boost include dirs: ${Boost_INCLUDE_DIRS}")
  message(STATUS "Boost static libraries: ${Boost_LIBRARIES}")

  install(FILES ${Boost_LIBRARIES} DESTINATION lib)
  install(DIRECTORY ${Boost_INCLUDE_DIRS}/boost DESTINATION include)
endif ()

if (USE_SYSTEM_AWS_SDK)
  find_package(aws-sdk-cpp REQUIRED)
else ()
  set(ZLIB_CXX_FLAGS "${EXTERNAL_CXX_FLAGS}")
  set(ZLIB_C_FLAGS "${EXTERNAL_C_FLAGS}")
  set(ZLIB_PREFIX "${PROJECT_BINARY_DIR}/external/zlib")
  set(ZLIB_INCLUDE_DIR "${ZLIB_PREFIX}/include")
  set(ZLIB_CMAKE_ARGS "-Wno-dev"
          "-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
          "-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}"
          "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
          "-DCMAKE_INSTALL_PREFIX=${ZLIB_PREFIX}"
          "-DBUILD_SHARED_LIBS=OFF")

  set(ZLIB_STATIC_LIB_NAME "${CMAKE_STATIC_LIBRARY_PREFIX}z")
  set(ZLIB_LIBRARY "${ZLIB_PREFIX}/lib/${ZLIB_STATIC_LIB_NAME}${CMAKE_STATIC_LIBRARY_SUFFIX}")
  ExternalProject_Add(zlib
          URL http://zlib.net/zlib-${ZLIB_VERSION}.tar.gz
          CMAKE_ARGS ${ZLIB_CMAKE_ARGS}
          LOG_DOWNLOAD ON
          LOG_CONFIGURE ON
          LOG_BUILD ON
          LOG_INSTALL ON)

  include_directories(SYSTEM ${ZLIB_INCLUDE_DIR})
  message(STATUS "ZLib include dir: ${ZLIB_INCLUDE_DIR}")
  message(STATUS "ZLib static library: ${ZLIB_LIBRARY}")

  install(FILES ${ZLIB_LIBRARY} DESTINATION lib)
  install(DIRECTORY ${ZLIB_INCLUDE_DIR}/ DESTINATION include)

  set(OPENSSL_CXX_FLAGS "${EXTERNAL_CXX_FLAGS}")
  set(OPENSSL_C_FLAGS "${EXTERNAL_C_FLAGS}")
  set(OPENSSL_PREFIX "${PROJECT_BINARY_DIR}/external/openssl")
  set(OPENSSL_INCLUDE_DIR "${OPENSSL_PREFIX}/include")
  set(OPENSSL_STATIC_LIB_NAME "${CMAKE_STATIC_LIBRARY_PREFIX}ssl")
  set(CRYPTO_STATIC_LIB_NAME "${CMAKE_STATIC_LIBRARY_PREFIX}crypto")
  set(OPENSSL_LIBRARIES "${OPENSSL_PREFIX}/lib/${OPENSSL_STATIC_LIB_NAME}${CMAKE_STATIC_LIBRARY_SUFFIX}"
          "${OPENSSL_PREFIX}/lib/${CRYPTO_STATIC_LIB_NAME}${CMAKE_STATIC_LIBRARY_SUFFIX}")
  ExternalProject_Add(openssl
          DEPENDS zlib
          URL https://www.openssl.org/source/openssl-${OPENSSL_VERSION}.tar.gz
          BUILD_IN_SOURCE 1
          CONFIGURE_COMMAND ./config --prefix=${OPENSSL_PREFIX} --with-zlib-include=${ZLIB_INCLUDE_DIR} --with-zlib-lib=${ZLIB_PREFIX}/lib no-shared no-tests CXX=${CMAKE_CXX_COMPILER} CC=${CMAKE_C_COMPILER} CFLAGS=${OPENSSL_C_FLAGS} CXXFLAGS=${OPENSSL_CXX_FLAGS}
          BUILD_COMMAND "$(MAKE)"
          INSTALL_COMMAND "$(MAKE)" install
          LOG_DOWNLOAD ON
          LOG_CONFIGURE ON
          LOG_BUILD ON
          LOG_INSTALL ON)

  include_directories(SYSTEM ${OPENSSL_INCLUDE_DIR})
  message(STATUS "OpenSSL include dir: ${OPENSSL_INCLUDE_DIR}")
  message(STATUS "OpenSSL static libraries: ${OPENSSL_LIBRARIES}")

  install(FILES ${OPENSSL_LIBRARIES} DESTINATION lib)
  install(DIRECTORY ${OPENSSL_INCLUDE_DIR}/openssl DESTINATION include)

  set(CURL_CXX_FLAGS "${EXTERNAL_CXX_FLAGS}")
  set(CURL_C_FLAGS "${EXTERNAL_C_FLAGS}")
  set(CURL_PREFIX "${PROJECT_BINARY_DIR}/external/curl")
  set(CURL_HOME "${CURL_PREFIX}")
  set(CURL_INCLUDE_DIR "${CURL_PREFIX}/include")
  set(CURL_PREFIX_PATH "${ZLIB_PREFIX}|${OPENSSL_PREFIX}")
  set(CURL_CMAKE_ARGS "-Wno-dev"
          "-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
          "-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}"
          "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
          "-DCMAKE_INSTALL_PREFIX=${CURL_PREFIX}"
          "-DCMAKE_PREFIX_PATH=${CURL_PREFIX_PATH}"
          "-DCURL_STATICLIB=ON"
          "-DBUILD_CURL_EXE=OFF"
          "-DBUILD_TESTING=OFF"
          "-DENABLE_MANUAL=OFF"
          "-DHTTP_ONLY=ON"
          "-DCURL_CA_PATH=none"
          "-DZLIB_LIBRARY=${ZLIB_LIBRARY}") # Force usage of static library

  set(CURL_STATIC_LIB_NAME "${CMAKE_STATIC_LIBRARY_PREFIX}curl")
  set(CURL_LIBRARY "${CURL_PREFIX}/lib/${CURL_STATIC_LIB_NAME}${CMAKE_STATIC_LIBRARY_SUFFIX}")
  string(REGEX REPLACE "\\." "_" CURL_VERSION_STR ${CURL_VERSION})
  ExternalProject_Add(curl
          DEPENDS zlib openssl
          URL https://github.com/curl/curl/releases/download/curl-${CURL_VERSION_STR}/curl-${CURL_VERSION}.tar.gz
          LIST_SEPARATOR |
          CMAKE_ARGS ${CURL_CMAKE_ARGS}
          LOG_DOWNLOAD ON
          LOG_CONFIGURE ON
          LOG_BUILD ON
          LOG_INSTALL ON)

  include_directories(SYSTEM ${CURL_INCLUDE_DIR})
  message(STATUS "Curl include dir: ${CURL_INCLUDE_DIR}")
  message(STATUS "Curl static library: ${CURL_LIBRARY}")

  install(FILES ${CURL_LIBRARY} DESTINATION lib)
  install(DIRECTORY ${CURL_INCLUDE_DIR}/curl DESTINATION include)

  set(AWS_CXX_FLAGS "${EXTERNAL_CXX_FLAGS}")
  set(AWS_C_FLAGS "${EXTERNAL_C_FLAGS}")
  set(AWS_PREFIX "${PROJECT_BINARY_DIR}/external/aws")
  set(AWS_HOME "${AWS_PREFIX}")
  set(AWS_BUILD_PROJECTS "s3|dynamodb")
  set(AWS_INCLUDE_DIR "${AWS_PREFIX}/include")
  set(AWS_PREFIX_PATH "${ZLIB_PREFIX}|${OPENSSL_PREFIX}|${CURL_PREFIX}")
  set(AWS_CMAKE_ARGS "-Wno-dev"
          "-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
          "-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}"
          "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
          "-DBUILD_ONLY=${AWS_BUILD_PROJECTS}"
          "-DCMAKE_INSTALL_PREFIX=${AWS_PREFIX}"
          "-DENABLE_TESTING=OFF"
          "-DBUILD_SHARED_LIBS=OFF"
          "-DCMAKE_PREFIX_PATH=${AWS_PREFIX_PATH}"
          "-DZLIB_LIBRARY=${ZLIB_LIBRARY}") # Force usage of static library

  set(AWS_STATIC_CORE_LIB_NAME "${CMAKE_STATIC_LIBRARY_PREFIX}aws-cpp-sdk-core")
  set(AWS_STATIC_S3_LIB_NAME "${CMAKE_STATIC_LIBRARY_PREFIX}aws-cpp-sdk-s3")
  set(AWS_STATIC_DYNAMODB_LIB_NAME "${CMAKE_STATIC_LIBRARY_PREFIX}aws-cpp-sdk-dynamodb")
  set(AWS_LIBRARIES "${AWS_PREFIX}/lib/${AWS_STATIC_S3_LIB_NAME}${CMAKE_STATIC_LIBRARY_SUFFIX}"
          "${AWS_PREFIX}/lib/${AWS_STATIC_DYNAMODB_LIB_NAME}${CMAKE_STATIC_LIBRARY_SUFFIX}"
          "${AWS_PREFIX}/lib/${AWS_STATIC_CORE_LIB_NAME}${CMAKE_STATIC_LIBRARY_SUFFIX}")

  ExternalProject_Add(awssdk
          DEPENDS curl openssl zlib
          GIT_REPOSITORY "https://github.com/awslabs/aws-sdk-cpp.git"
          GIT_TAG "${AWSSDK_VERSION}"
          GIT_SHALLOW 1
          BUILD_IN_SOURCE true
          LIST_SEPARATOR |
          CMAKE_ARGS ${AWS_CMAKE_ARGS}
          LOG_DOWNLOAD ON
          LOG_CONFIGURE ON
          LOG_BUILD ON
          LOG_INSTALL ON)

  include_directories(SYSTEM ${AWS_INCLUDE_DIR})
  message(STATUS "AWS include dir: ${AWS_INCLUDE_DIR}")
  message(STATUS "AWS static libraries: ${AWS_LIBRARIES}")

  install(FILES ${AWS_LIBRARIES} DESTINATION lib)
  install(DIRECTORY ${AWS_INCLUDE_DIR}/aws DESTINATION include)
endif ()

if (NOT USE_SYSTEM_JEMALLOC)
  set(JEMALLOC_CXX_FLAGS "${EXTERNAL_CXX_FLAGS}")
  set(JEMALLOC_C_FLAGS "${EXTERNAL_C_FLAGS}")
  set(JEMALLOC_LD_FLAGS "-Wl,--no-as-needed")
  set(JEMALLOC_PREFIX "${PROJECT_BINARY_DIR}/external/jemalloc")
  set(JEMALLOC_HOME "${JEMALLOC_PREFIX}")
  set(JEMALLOC_INCLUDE_DIR "${JEMALLOC_PREFIX}/include")
  set(JEMALLOC_CMAKE_ARGS "-Wno-dev"
          "-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
          "-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}"
          "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
          "-DCMAKE_CXX_FLAGS=${JEMALLOC_CXX_FLAGS}"
          "-DCMAKE_INSTALL_PREFIX=${JEMALLOC_PREFIX}")
  set(JEMALLOC_STATIC_LIB_NAME "${CMAKE_STATIC_LIBRARY_PREFIX}jemalloc")
  set(JEMALLOC_LIBRARIES "${JEMALLOC_PREFIX}/lib/${JEMALLOC_STATIC_LIB_NAME}${CMAKE_STATIC_LIBRARY_SUFFIX}")
  ExternalProject_Add(jemalloc
          URL https://github.com/jemalloc/jemalloc/releases/download/5.0.1/jemalloc-5.0.1.tar.bz2
          PREFIX ${JEMALLOC_PREFIX}
          BUILD_BYPRODUCTS ${JEMALLOC_LIBRARIES}
          CONFIGURE_COMMAND ${JEMALLOC_PREFIX}/src/jemalloc/configure --prefix=${JEMALLOC_PREFIX} --enable-autogen --enable-prof-libunwind CFLAGS=${JEMALLOC_C_FLAGS} CXXFLAGS=${JEMALLOC_CXX_FLAGS}
          INSTALL_COMMAND make install_lib
          LOG_DOWNLOAD ON
          LOG_CONFIGURE ON
          LOG_BUILD ON
          LOG_INSTALL ON)
  message(STATUS "Jemalloc library: ${JEMALLOC_LIBRARIES}")
  install(FILES ${JEMALLOC_LIBRARIES} DESTINATION lib)
endif ()

if (NOT USE_SYSTEM_CPP_REDIS)
  set(CPP_REDIS_CXX_FLAGS "${EXTERNAL_CXX_FLAGS}")
  set(CPP_REDIS_C_FLAGS "${EXTERNAL_C_FLAGS}")
  set(CPP_REDIS_PREFIX "${PROJECT_BINARY_DIR}/external/cpp_redis")
  set(CPP_REDIS_HOME "${CPP_REDIS_PREFIX}")
  set(CPP_REDIS_INCLUDE_DIR "${CPP_REDIS_PREFIX}/include")
  set(CPP_REDIS_CMAKE_ARGS "-Wno-dev"
          "-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
          "-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}"
          "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
          "-DCMAKE_INSTALL_PREFIX=${CPP_REDIS_PREFIX}"
          "-DBUILD_EXAMPLES=OFF"
          "-DBUILD_TESTS=OFF"
          "-DBUILD_SHARED_LIBS=OFF")
  set(CPP_REDIS_STATIC_LIB_NAME "${CMAKE_STATIC_LIBRARY_PREFIX}cpp_redis")
  set(CPP_REDIS_TACOPIE_STATIC_LIB_NAME "${CMAKE_STATIC_LIBRARY_PREFIX}tacopie")
  set(CPP_REDIS_LIBRARY "${CPP_REDIS_PREFIX}/lib/${CPP_REDIS_STATIC_LIB_NAME}${CMAKE_STATIC_LIBRARY_SUFFIX}")
  set(CPP_REDIS_TACOPIE_LIBRARY "${CPP_REDIS_PREFIX}/lib/${CPP_REDIS_TACOPIE_STATIC_LIB_NAME}${CMAKE_STATIC_LIBRARY_SUFFIX}")
  set(CPP_REDIS_LIBRARIES ${CPP_REDIS_LIBRARY} ${CPP_REDIS_TACOPIE_LIBRARY})
  ExternalProject_Add(cpp_redis
          GIT_REPOSITORY https://github.com/Cylix/cpp_redis.git
          GIT_TAG ${CPP_REDIS_VERSION}
          GIT_SUBMODULES
          LIST_SEPARATOR |
          CMAKE_ARGS ${CPP_REDIS_CMAKE_ARGS}
          LOG_DOWNLOAD ON
          LOG_CONFIGURE ON
          LOG_BUILD ON
          LOG_INSTALL ON)
  include_directories(SYSTEM ${CPP_REDIS_INCLUDE_DIR})
  message(STATUS "cpp_redis include dir: ${CPP_REDIS_INCLUDE_DIR}")
  message(STATUS "cpp_redis static library: ${CPP_REDIS_LIBRARIES}")

  install(FILES ${CPP_REDIS_LIBRARIES} DESTINATION lib)
  install(DIRECTORY ${CPP_REDIS_INCLUDE_DIR}/cpp_redis DESTINATION include)
endif ()