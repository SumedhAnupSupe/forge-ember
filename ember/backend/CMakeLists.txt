cmake_minimum_required(VERSION 3.16)
project(ember_backend VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(Threads REQUIRED)
# Reuse the HTTP and JSON headers already vendored by FORGE; do not link to its storage layer.
add_executable(ember-server src/main.cpp src/server.cpp src/forge_client.cpp)
target_include_directories(ember-server PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/../../forge/third_party
)
target_link_libraries(ember-server PRIVATE Threads::Threads)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(ember-server PRIVATE -Wall -Wextra -Wpedantic)
endif()
enable_testing()
