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

add_executable(ark
    src/main.cpp

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

target_include_directories(ark PUBLIC ${CMAKE_SOURCE_DIR}/include)

find_package(OpenSSL REQUIRED)
target_link_libraries(ark PRIVATE OpenSSL::SSL OpenSSL::Crypto)

find_package(ZLIB REQUIRED)
target_link_libraries(ark PRIVATE ZLIB::ZLIB)

if (APPLE)
    target_compile_definitions(ark PRIVATE _DARWIN_C_SOURCE)
endif()

install(TARGETS ark RUNTIME DESTINATION bin)

add_executable(test_ark
    tests/test_main.cpp
    tests/test_logger.cpp
    tests/test_config.cpp
    tests/test_repository.cpp
    tests/test_ref.cpp
    tests/test_index.cpp
    tests/test_objects.cpp
    tests/test_e2e.cpp
    
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
    src/commands/init.cpp
    src/commands/switch.cpp
    
    src/utils/compress.cpp
    src/utils/objects.cpp
    src/utils/ark.cpp
    src/utils/index.cpp
    src/utils/config.cpp
    src/utils/repository.cpp
    src/utils/ref.cpp
    src/utils/logger.cpp
)

target_include_directories(test_ark PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(test_ark PRIVATE OpenSSL::SSL OpenSSL::Crypto ZLIB::ZLIB gtest)

if (APPLE)
    target_compile_definitions(test_ark PRIVATE _DARWIN_C_SOURCE)
endif()

add_test(NAME ArkUnitTests COMMAND test_ark)
