cmake_minimum_required(VERSION 3.10)

add_executable(forge_tests
    test_main.cpp
    test_codec.cpp
    test_storage.cpp
    test_services.cpp
    test_auth.cpp
    test_hash_table.cpp
    test_sorting.cpp
    test_query.cpp
    test_integration.cpp
)

target_include_directories(forge_tests PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(forge_tests PRIVATE forge_core Threads::Threads)

if(ARGON2_LIBRARY AND ARGON2_INCLUDE_DIR)
    target_link_libraries(forge_tests PRIVATE ${ARGON2_LIBRARY})
elseif(EXISTS "/usr/lib/x86_64-linux-gnu/libargon2.so.1")
    find_path(ARGON2_INCLUDE_DIR_CANDIDATE argon2.h PATHS /usr/include /usr/local/include)
    if(ARGON2_INCLUDE_DIR_CANDIDATE)
        target_link_libraries(forge_tests PRIVATE /usr/lib/x86_64-linux-gnu/libargon2.so.1)
    endif()
endif()

add_test(NAME forge_tests COMMAND forge_tests)
