cmake_minimum_required(VERSION 3.20)

project(llmcpp VERSION 0.1 LANGUAGES CXX)

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/build/install" CACHE PATH
      "Installation directory" FORCE)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(CTest)

set(LLMCPP_LLVM_VERSION 19 CACHE STRING "LLVM/Clang major version")
set(LLMCPP_DEPS_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/deps/root")
set(LLMCPP_DEPS_LIBDIR "${LLMCPP_DEPS_ROOT}/usr/lib/x86_64-linux-gnu")
set(LLMCPP_CATCH2_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/deps/catch2/src")
set(LLMCPP_CPPHTTPLIB_INCLUDE_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/deps/cpp-httplib/include")
if(NOT EXISTS "${LLMCPP_DEPS_ROOT}/.fetched-${LLMCPP_LLVM_VERSION}" OR
   NOT EXISTS "${LLMCPP_CATCH2_SOURCE_DIR}/CMakeLists.txt" OR
   NOT EXISTS "${LLMCPP_CPPHTTPLIB_INCLUDE_DIR}/httplib.h")
  message(FATAL_ERROR
    "Dependencies are missing. Run ./fetch-deps.sh before configuring llmcpp.")
endif()

set(LLMCPP_LLVM_INCLUDE_DIR
    "${LLMCPP_DEPS_ROOT}/usr/include/llvm-${LLMCPP_LLVM_VERSION}")
set(LLMCPP_LLVM_C_INCLUDE_DIR
    "${LLMCPP_DEPS_ROOT}/usr/include/llvm-c-${LLMCPP_LLVM_VERSION}")
set(LLMCPP_CLANG_INCLUDE_DIR
    "${LLMCPP_DEPS_ROOT}/usr/lib/llvm-${LLMCPP_LLVM_VERSION}/include")
set(LLMCPP_CLANG_RESOURCE_DIR
    "${LLMCPP_DEPS_ROOT}/usr/lib/llvm-${LLMCPP_LLVM_VERSION}/lib/clang/${LLMCPP_LLVM_VERSION}/include")

find_file(LLMCPP_CLANG_CPP_LIBRARY
  NAMES "libclang-cpp.so.${LLMCPP_LLVM_VERSION}.1"
  PATHS "${LLMCPP_DEPS_LIBDIR}"
  NO_DEFAULT_PATH
)
find_file(LLMCPP_LLVM_LIBRARY
  NAMES "libLLVM.so.${LLMCPP_LLVM_VERSION}.1"
  PATHS
    "${LLMCPP_DEPS_LIBDIR}"
    "/usr/lib/x86_64-linux-gnu"
    "/lib/x86_64-linux-gnu"
  NO_DEFAULT_PATH
)
if(NOT LLMCPP_CLANG_CPP_LIBRARY OR NOT LLMCPP_LLVM_LIBRARY)
  message(FATAL_ERROR "LLVM/Clang runtime libraries were not found")
endif()
cmake_path(IS_PREFIX LLMCPP_DEPS_ROOT "${LLMCPP_LLVM_LIBRARY}" NORMALIZE
           LLMCPP_LLVM_IS_BUNDLED)

find_package(Threads REQUIRED)
set(OPENSSL_ROOT_DIR "${LLMCPP_DEPS_ROOT}/usr")
set(OPENSSL_INCLUDE_DIR "${LLMCPP_DEPS_ROOT}/usr/include")
set(OPENSSL_SSL_LIBRARY "${LLMCPP_DEPS_LIBDIR}/libssl.so")
set(OPENSSL_CRYPTO_LIBRARY "${LLMCPP_DEPS_LIBDIR}/libcrypto.so")
find_package(OpenSSL REQUIRED COMPONENTS SSL Crypto)

add_executable(llmcxx
  src/llmcpp/agent_anthropic_client.cpp
  src/llmcpp/agent_session.cpp
  src/llmcpp/agent_tool_server.cpp
  src/llmcpp/compiler_ast_text.cpp
  src/llmcpp/compiler_type_probe_finder.cpp
  src/llmcpp/compiler_type_inspector.cpp
  src/llmcpp/frontend_source_collector.cpp
  src/llmcpp/frontend_candidate_collector.cpp
  src/llmcpp/compiler_diagnostic_collector.cpp
  src/llmcpp/driver_frontend_runner.cpp
  src/llmcpp/frontend_guard_action.cpp
  src/llmcpp/compiler_inspection_action.cpp
  src/llmcpp/compiler_inspection_consumer.cpp
  src/llmcpp/frontend_header_annotation_guard.cpp
  src/llmcpp/frontend_generation_action.cpp
  src/llmcpp/frontend_generation_consumer.cpp
  src/llmcpp/compiler_sandbox.cpp
  src/llmcpp/driver_app.cpp
  src/llmcpp/driver_options.cpp
  src/llmcpp/generation_entry_point.cpp
  src/llmcpp/generation_pass.cpp
  src/llmcpp/generation_context.cpp
  src/llmcpp/source_text.cpp
  src/main.cpp
)
set_target_properties(llmcxx PROPERTIES
  OUTPUT_NAME "llmc++"
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
  BUILD_WITH_INSTALL_RPATH TRUE
  INSTALL_RPATH "$ORIGIN/../lib"
)
target_compile_features(llmcxx PRIVATE cxx_std_17)
target_compile_options(llmcxx PRIVATE -fno-rtti -Wall -Wno-unused-parameter)
target_compile_definitions(llmcxx PRIVATE
  _GNU_SOURCE
  __STDC_CONSTANT_MACROS
  __STDC_FORMAT_MACROS
  __STDC_LIMIT_MACROS
  LLMCPP_LLVM_VERSION="${LLMCPP_LLVM_VERSION}"
  CPPHTTPLIB_OPENSSL_SUPPORT
)
target_include_directories(llmcxx SYSTEM PRIVATE
  "${LLMCPP_CPPHTTPLIB_INCLUDE_DIR}"
  "${LLMCPP_DEPS_ROOT}/usr/include/x86_64-linux-gnu"
  "${LLMCPP_LLVM_INCLUDE_DIR}"
  "${LLMCPP_LLVM_C_INCLUDE_DIR}"
  "${LLMCPP_CLANG_INCLUDE_DIR}"
)
target_include_directories(llmcxx PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(llmcxx PRIVATE
  "${LLMCPP_CLANG_CPP_LIBRARY}"
  "${LLMCPP_LLVM_LIBRARY}"
  OpenSSL::SSL
  OpenSSL::Crypto
  Threads::Threads
)

set(LLMCPP_AGENT "${CMAKE_BINARY_DIR}/bin/llmcpp-agent")
add_custom_command(
  OUTPUT "${LLMCPP_AGENT}"
  COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_BINARY_DIR}/bin"
  COMMAND "${CMAKE_COMMAND}" -E copy_if_different
          "${CMAKE_CURRENT_SOURCE_DIR}/agent/llmcpp-agent" "${LLMCPP_AGENT}"
  COMMAND chmod +x "${LLMCPP_AGENT}"
  DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/agent/llmcpp-agent"
  VERBATIM
)
add_custom_target(llmcpp-agent ALL DEPENDS "${LLMCPP_AGENT}")
add_dependencies(llmcxx llmcpp-agent)

install(TARGETS llmcxx RUNTIME DESTINATION bin)
install(PROGRAMS "${CMAKE_CURRENT_SOURCE_DIR}/agent/llmcpp-agent" DESTINATION bin)
install(FILES "${LLMCPP_CLANG_CPP_LIBRARY}" DESTINATION lib)
install(DIRECTORY "${LLMCPP_CLANG_RESOURCE_DIR}/"
        DESTINATION "lib/clang/${LLMCPP_LLVM_VERSION}/include")
if(LLMCPP_LLVM_IS_BUNDLED)
  install(FILES "${LLMCPP_LLVM_LIBRARY}" DESTINATION lib)
endif()

add_custom_command(TARGET llmcxx POST_BUILD
  COMMAND "${CMAKE_COMMAND}" -E make_directory
          "${CMAKE_BINARY_DIR}/lib"
          "${CMAKE_BINARY_DIR}/lib/clang/${LLMCPP_LLVM_VERSION}"
  COMMAND "${CMAKE_COMMAND}" -E copy_if_different
          "${LLMCPP_CLANG_CPP_LIBRARY}"
          "${CMAKE_BINARY_DIR}/lib/libclang-cpp.so.${LLMCPP_LLVM_VERSION}.1"
  COMMAND "${CMAKE_COMMAND}" -E copy_directory
          "${LLMCPP_CLANG_RESOURCE_DIR}"
          "${CMAKE_BINARY_DIR}/lib/clang/${LLMCPP_LLVM_VERSION}/include"
  VERBATIM
)
if(LLMCPP_LLVM_IS_BUNDLED)
  add_custom_command(TARGET llmcxx POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${LLMCPP_LLVM_LIBRARY}"
            "${CMAKE_BINARY_DIR}/lib/libLLVM.so.${LLMCPP_LLVM_VERSION}.1"
    VERBATIM
  )
endif()

if(BUILD_TESTING)
  add_subdirectory("${LLMCPP_CATCH2_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/catch2" EXCLUDE_FROM_ALL)

  add_executable(llmcpp-tests test/integration_tests.cpp)
  set_target_properties(llmcpp-tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
  )
  target_compile_features(llmcpp-tests PRIVATE cxx_std_17)
  target_include_directories(llmcpp-tests SYSTEM PRIVATE
    "${LLMCPP_CPPHTTPLIB_INCLUDE_DIR}"
  )
  target_link_libraries(llmcpp-tests PRIVATE Catch2::Catch2WithMain)
  target_compile_definitions(llmcpp-tests PRIVATE
    LLMCXX_PATH="$<TARGET_FILE:llmcxx>"
    MOCK_AGENT_PATH="${CMAKE_CURRENT_SOURCE_DIR}/test/mock-agent/llmcpp-mock-agent"
    TEST_CASES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/test/cases"
  )
  add_dependencies(llmcpp-tests llmcxx)
  install(TARGETS llmcpp-tests RUNTIME DESTINATION bin)

  list(APPEND CMAKE_MODULE_PATH "${LLMCPP_CATCH2_SOURCE_DIR}/extras")
  include(Catch)
  catch_discover_tests(llmcpp-tests
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    PROPERTIES TIMEOUT 180
  )
endif()
