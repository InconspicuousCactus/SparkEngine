# ==============================================
# Project Definition and options
# ==============================================
cmake_minimum_required(VERSION 3.20 FATAL_ERROR)
project(spark_tests)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Setup engine
cmake_path(GET CMAKE_CURRENT_SOURCE_DIR PARENT_PATH spark_dir)
message("Spark dir: ${spark_dir}")

file(GLOB_RECURSE SOURCE_FILES 
    ${CMAKE_CURRENT_SOURCE_DIR}/*.c
)

add_executable(spark_tests ${SOURCE_FILES})
target_include_directories(spark_tests PRIVATE "${spark_dir}/include")
