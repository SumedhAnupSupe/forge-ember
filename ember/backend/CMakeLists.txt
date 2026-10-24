cmake_minimum_required(VERSION 3.16)
project(ember_backend VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)

include(FetchContent)
FetchContent_Declare(
    cpp_httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG v0.18.3
)
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.12.0
)
FetchContent_MakeAvailable(cpp_httplib nlohmann_json)

add_executable(ember-server
    src/main.cpp
    src/server.cpp
    src/forge_client.cpp
)
target_include_directories(ember-server PRIVATE include)
target_link_libraries(ember-server PRIVATE httplib::httplib nlohmann_json::nlohmann_json Threads::Threads)

include(CTest)
if(BUILD_TESTING)
    add_executable(ember-unit-tests tests/unit_tests.cpp)
    target_link_libraries(ember-unit-tests PRIVATE nlohmann_json::nlohmann_json)
    add_test(NAME ember-unit-tests COMMAND ember-unit-tests)
endif()
