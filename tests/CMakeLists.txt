# Tests CMakeLists
cmake_minimum_required(VERSION 3.10)

# Find test framework (simple approach)
file(GLOB_RECURSE TEST_SOURCES "${PROJECT_SOURCE_DIR}/src/*.cpp")
list(REMOVE_ITEM TEST_SOURCES "${PROJECT_SOURCE_DIR}/src/main.cpp")
add_executable(tests test_main.cpp ${TEST_SOURCES})
target_include_directories(tests PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(tests Threads::Threads)

if(ARGON2_LIBRARY AND ARGON2_INCLUDE_DIR)
    target_link_libraries(tests ${ARGON2_LIBRARY})
elseif(EXISTS "/usr/lib/x86_64-linux-gnu/libargon2.so.1")
    target_link_libraries(tests /usr/lib/x86_64-linux-gnu/libargon2.so.1)
endif()
add_test(NAME forge_tests COMMAND tests)
