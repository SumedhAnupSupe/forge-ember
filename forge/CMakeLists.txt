cmake_minimum_required(VERSION 3.10)
project(Forge VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-Wall -Wextra)
endif()

include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/third_party)
include_directories(${PROJECT_SOURCE_DIR}/include/external)

find_package(Threads REQUIRED)

# Production sources
set(FORGE_CORE_SOURCES
    src/models/records.cpp
    src/utils/codec.cpp
    src/storage/storage.cpp
    src/index/hash_table.cpp
    src/index/indexes.cpp
    src/services/user_service.cpp
    src/services/message_service.cpp
    src/auth/auth.cpp
    src/algorithms/searching.cpp
    src/algorithms/sorting.cpp
    src/algorithms/benchmark.cpp
    src/query/query.cpp
)

add_library(forge_core ${FORGE_CORE_SOURCES})
target_link_libraries(forge_core PUBLIC Threads::Threads)

find_library(ARGON2_LIBRARY argon2)
find_path(ARGON2_HEADER argon2.h PATHS ${PROJECT_SOURCE_DIR}/include/external /usr/include /usr/local/include)
if(ARGON2_LIBRARY AND ARGON2_HEADER)
    target_include_directories(forge_core PUBLIC ${ARGON2_HEADER})
    target_link_libraries(forge_core PUBLIC ${ARGON2_LIBRARY})
    target_compile_definitions(forge_core PUBLIC HAVE_ARGON2)
elseif(EXISTS "/usr/lib/x86_64-linux-gnu/libargon2.so.1" AND EXISTS "${PROJECT_SOURCE_DIR}/include/external/argon2.h")
    target_include_directories(forge_core PUBLIC ${PROJECT_SOURCE_DIR}/include/external)
    target_link_libraries(forge_core PUBLIC /usr/lib/x86_64-linux-gnu/libargon2.so.1)
    target_compile_definitions(forge_core PUBLIC HAVE_ARGON2)
else()
    message(FATAL_ERROR "Argon2id required: libargon2 and argon2.h not found")
endif()

# Server
add_executable(forge-server src/server/main_server.cpp src/server/server.cpp)
target_link_libraries(forge-server PRIVATE forge_core Threads::Threads)

# CLI
add_executable(forge-cli src/cli/cli_main.cpp src/cli/cli.cpp)
target_link_libraries(forge-cli PRIVATE Threads::Threads)

# Original forge executable
add_executable(forge src/main.cpp)
target_link_libraries(forge PRIVATE forge_core)

enable_testing()
add_subdirectory(tests)
