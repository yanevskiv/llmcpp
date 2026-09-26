cmake_minimum_required(VERSION 3.20)

project(llmcpp VERSION 0.1 LANGUAGES CXX)

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/build/install" CACHE PATH
      "Installation directory" FORCE)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(CTest)
include(FetchContent)
if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()

set(LLMCPP_LLVM_VERSION 19 CACHE STRING "LLVM/Clang major version")
set(LLMCPP_DEPS_ROOT "${CMAKE_BINARY_DIR}/deps/root")
set(LLMCPP_DEPS_LIBDIR "${LLMCPP_DEPS_ROOT}/usr/lib/x86_64-linux-gnu")

execute_process(
  COMMAND "${CMAKE_COMMAND}" -E env
          "LLVM_VER=${LLMCPP_LLVM_VERSION}"
          "LLMCPP_DEPS_DIR=${CMAKE_BINARY_DIR}/deps"
          "${CMAKE_CURRENT_SOURCE_DIR}/scripts/fetch-deps.sh"
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
  RESULT_VARIABLE LLMCPP_FETCH_RESULT
)
if(NOT LLMCPP_FETCH_RESULT EQUAL 0)
  message(FATAL_ERROR "Failed to fetch LLVM/Clang ${LLMCPP_LLVM_VERSION} dependencies")
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

find_package(Threads REQUIRED)

add_executable(llmcxx
  src/llmcpp/agent/agent_session.cpp
  src/llmcpp/agent/tool_server.cpp
  src/llmcpp/compiler/ast_text.cpp
  src/llmcpp/compiler/probe_finder.cpp
  src/llmcpp/compiler/type_tools.cpp
  src/llmcpp/frontend/callbacks.cpp
  src/llmcpp/frontend/candidate_collector.cpp
  src/llmcpp/compiler/collector.cpp
  src/llmcpp/driver/frontend_runner.cpp
  src/llmcpp/frontend/guard_action.cpp
  src/llmcpp/compiler/inspect_action.cpp
  src/llmcpp/compiler/inspect_consumer.cpp
  src/llmcpp/frontend/keyword_guard.cpp
  src/llmcpp/frontend/pass_action.cpp
  src/llmcpp/frontend/pass_consumer.cpp
  src/llmcpp/compiler/shadow_compiler.cpp
  src/llmcpp/driver/driver_app.cpp
  src/llmcpp/driver/driver_options.cpp
  src/llmcpp/generation/llm_pass.cpp
  src/llmcpp/generation/pass.cpp
  src/llmcpp/generation/pass_state.cpp
  src/llmcpp/source/text.cpp
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
)
target_include_directories(llmcxx SYSTEM PRIVATE
  "${LLMCPP_LLVM_INCLUDE_DIR}"
  "${LLMCPP_LLVM_C_INCLUDE_DIR}"
  "${LLMCPP_CLANG_INCLUDE_DIR}"
)
target_include_directories(llmcxx PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(llmcxx PRIVATE
  "${LLMCPP_CLANG_CPP_LIBRARY}"
  "${LLMCPP_LLVM_LIBRARY}"
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
        DESTINATION "lib/clang/${LLMCPP_LLVM_VERSION}")
if(LLMCPP_LLVM_LIBRARY MATCHES "^${LLMCPP_DEPS_ROOT}")
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
if(LLMCPP_LLVM_LIBRARY MATCHES "^${LLMCPP_DEPS_ROOT}")
  add_custom_command(TARGET llmcxx POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${LLMCPP_LLVM_LIBRARY}"
            "${CMAKE_BINARY_DIR}/lib/libLLVM.so.${LLMCPP_LLVM_VERSION}.1"
    VERBATIM
  )
endif()

if(BUILD_TESTING)
  find_package(Catch2 3 QUIET)
  if(NOT Catch2_FOUND)
    FetchContent_Declare(Catch2
      URL https://github.com/catchorg/Catch2/archive/refs/tags/v3.8.1.tar.gz
    )
    FetchContent_MakeAvailable(Catch2)
  endif()

  add_executable(llmcpp-tests test/integration_tests.cpp)
  set_target_properties(llmcpp-tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
  )
  target_compile_features(llmcpp-tests PRIVATE cxx_std_17)
  target_link_libraries(llmcpp-tests PRIVATE Catch2::Catch2WithMain)
  target_compile_definitions(llmcpp-tests PRIVATE
    LLMCXX_PATH="$<TARGET_FILE:llmcxx>"
    MOCK_AGENT_PATH="${CMAKE_CURRENT_SOURCE_DIR}/test/mock-agent/llmcpp-mock-agent"
    TEST_CASES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/test/cases"
  )
  add_dependencies(llmcpp-tests llmcxx)
  install(TARGETS llmcpp-tests RUNTIME DESTINATION bin)

  if(DEFINED catch2_SOURCE_DIR)
    list(APPEND CMAKE_MODULE_PATH "${catch2_SOURCE_DIR}/extras")
  elseif(DEFINED Catch2_DIR)
    list(APPEND CMAKE_MODULE_PATH "${Catch2_DIR}")
  endif()
  include(Catch)
  catch_discover_tests(llmcpp-tests
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    PROPERTIES TIMEOUT 180
  )
endif()
