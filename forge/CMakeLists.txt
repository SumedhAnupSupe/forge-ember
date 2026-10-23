cmake_minimum_required(VERSION 3.10)
project(Forge VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${PROJECT_SOURCE_DIR}/include)

find_package(Threads REQUIRED)

find_library(ARGON2_LIBRARY argon2)
find_path(ARGON2_INCLUDE_DIR argon2.h)

file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS "include/*.h" "include/*.hpp")

add_executable(forge ${SOURCES})
target_link_libraries(forge Threads::Threads)

if(ARGON2_LIBRARY AND ARGON2_INCLUDE_DIR)
    target_include_directories(forge PRIVATE ${ARGON2_INCLUDE_DIR})
    target_link_libraries(forge ${ARGON2_LIBRARY})
    add_definitions(-DHAVE_ARGON2)
elseif(EXISTS "/usr/lib/x86_64-linux-gnu/libargon2.so.1")
    find_path(ARGON2_INCLUDE_DIR_CANDIDATE argon2.h PATHS /usr/include /usr/local/include)
    if(ARGON2_INCLUDE_DIR_CANDIDATE)
        target_include_directories(forge PRIVATE ${ARGON2_INCLUDE_DIR_CANDIDATE})
        target_link_libraries(forge /usr/lib/x86_64-linux-gnu/libargon2.so.1)
        add_definitions(-DHAVE_ARGON2)
    else()
        # Headers not available - do not enable Argon2 usage; code will fail closed
    endif()
endif()

set(SOURCES ${SOURCES} PARENT_SCOPE)

enable_testing()
add_subdirectory(tests)
