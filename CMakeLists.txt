cmake_minimum_required(VERSION 3.29)
project(ionsl)

set(CMAKE_CXX_STANDARD 23)

file(GLOB_RECURSE
        SRC_FILES
        CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
)

add_library(ionsl STATIC ${SRC_FILES} include/ionsl/ionsl.h)
target_include_directories(ionsl PUBLIC include)

file(GLOB_RECURSE
        TEST_FILES
        CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.h
)

add_executable(ionsl_tests ${TEST_FILES})
target_link_libraries(ionsl_tests PUBLIC ionsl)
