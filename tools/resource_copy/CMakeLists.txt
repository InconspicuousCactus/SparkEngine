# ==============================================
# Project Definition and options
# ==============================================
cmake_minimum_required(VERSION 3.20 FATAL_ERROR)
project(resource_copy)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Add source files
file(GLOB_RECURSE SOURCE_FILES 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})
target_include_directories(resource_copy PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../include")

target_link_libraries(${PROJECT_NAME} PRIVATE assimp)
