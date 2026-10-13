cmake_minimum_required(VERSION 3.16)
project(ark LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
set(gtest_force_shared_crt ON)
FetchContent_MakeAvailable(googletest)

enable_testing()

option(ARK_SANITIZE "Build with AddressSanitizer + UndefinedBehaviorSanitizer" OFF)
if (ARK_SANITIZE)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer -g)
    add_link_options(-fsanitize=address,undefined)
endif()

find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)

# Everything except main() lives in a library so the CLI and the tests share one build.
add_library(ark_core STATIC
    src/commands/init.cpp
    src/commands/hash-object.cpp
    src/commands/cat-file.cpp
    src/commands/write-tree.cpp
    src/commands/commit-tree.cpp
    src/commands/add.cpp
    src/commands/commit.cpp
    src/commands/branch.cpp
    src/commands/switch.cpp
    src/commands/log.cpp
    src/commands/status.cpp
    src/commands/update-ref.cpp

    src/utils/compress.cpp
    src/utils/objects.cpp
    src/utils/ark.cpp
    src/utils/index.cpp
    src/utils/config.cpp
    src/utils/repository.cpp
    src/utils/ref.cpp
    src/utils/logger.cpp
)
target_include_directories(ark_core PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(ark_core PUBLIC OpenSSL::SSL OpenSSL::Crypto ZLIB::ZLIB)
if (APPLE)
    target_compile_definitions(ark_core PUBLIC _DARWIN_C_SOURCE)
endif()

add_executable(ark src/main.cpp)
target_link_libraries(ark PRIVATE ark_core)

install(TARGETS ark RUNTIME DESTINATION bin)

add_executable(test_ark
    tests/test_main.cpp
    tests/support/ark_test.cpp

    tests/unit/compress_test.cpp
    tests/unit/objects_test.cpp
    tests/unit/index_test.cpp
    tests/unit/config_test.cpp
    tests/unit/ref_test.cpp
    tests/unit/repository_test.cpp
    tests/unit/logger_test.cpp

    tests/cli/init_test.cpp
    tests/cli/plumbing_test.cpp
    tests/cli/add_test.cpp
    tests/cli/commit_test.cpp
    tests/cli/status_test.cpp
    tests/cli/branch_switch_test.cpp
    tests/cli/log_test.cpp

    tests/property/random_history_test.cpp
)
target_include_directories(test_ark PRIVATE ${CMAKE_SOURCE_DIR}/tests)
target_link_libraries(test_ark PRIVATE ark_core GTest::gtest)
# CLI tests run the real binary; make sure it is built and tell the tests where it is.
add_dependencies(test_ark ark)
target_compile_definitions(test_ark PRIVATE ARK_BIN="$<TARGET_FILE:ark>")

include(GoogleTest)
gtest_discover_tests(test_ark DISCOVERY_MODE PRE_TEST PROPERTIES TIMEOUT 120)
