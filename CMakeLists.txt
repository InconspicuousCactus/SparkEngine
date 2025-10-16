# ==============================================
# Project Definition and options
# ==============================================
cmake_minimum_required(VERSION 3.31 FATAL_ERROR)
project(SparkCore)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(SPARK_BUILD_TESTS OFF)
option(SPARK_LINK_STATIC OFF)
option(SPARK_DEBUG OFF)
option(SPARK_NO_ERROR_CHECKING OFF)
option(SPARK_CORE_ONLY OFF)
option(SPARK_WARNINGS_AS_ERRORS OFF)

message("===========================")
message("           SPARK           ")
message("===========================")

# Add source files
file(GLOB_RECURSE SOURCE_FILES 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/*.c
)

# Compiler flags
IF ((CMAKE_BUILD_TYPE MATCHES DEBUG) OR (SPARK_DEBUG))
    set(SPARK_DEBUG ON)
    message("debug mode")
ENDIF()
message("Compile flags: ${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS ${CMAKE_C_FLAGS})

# Define the link libraries
if (SPARK_LINK_STATIC)
    add_library(SparkCore STATIC ${SOURCE_FILES})
    message("Linking to spark statically")
else()
    add_library(SparkCore SHARED ${SOURCE_FILES})
    message("Linking to spark dynamically: " ${SPARK_LINK_STATIC} " : " ${SPARK_DEBUG})
endif()


# Dependencies
set(OBJECT_LAYER_BITS 32)
add_subdirectory(dependencies/joltc)
target_compile_options(joltc PRIVATE "-w")

# Define the include DIRs
target_include_directories(SparkCore PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include/"
    "${CMAKE_CURRENT_SOURCE_DIR}/api/"
    "${CMAKE_CURRENT_SOURCE_DIR}/dependencies/glfw/include/"
    "${CMAKE_CURRENT_SOURCE_DIR}/dependencies/glad/include/"
    "${CMAKE_CURRENT_SOURCE_DIR}/dependencies/stb"
    "${CMAKE_CURRENT_SOURCE_DIR}/dependencies/joltc/include"
)

# target_compile_options(SparkCore PRIVATE -Werror)
target_precompile_headers(SparkCore PRIVATE 
    "$<$<COMPILE_LANGUAGE:C>:${CMAKE_CURRENT_SOURCE_DIR}/include/Spark/PCH.h>"
)

if (SPARK_NO_ERROR_CHECKING) 
    message("Spark not compiling with error checking (Asserts / SERROR)")
    target_compile_definitions(SparkCore PUBLIC "SPARK_NO_ERROR_CHECKING")
endif()
if (SPARK_WARNINGS_AS_ERRORS) 
    target_compile_options(SparkCore PRIVATE "-Wall" "-Werror")
endif()
if (SPARK_DEBUG) 
    message("adding spark_debug preprocessing: " ${SPARK_DEBUG})
    target_compile_definitions(SparkCore PUBLIC "SPARK_DEBUG")
endif()

# NOTE: m is the c math library
target_link_libraries(SparkCore PRIVATE vulkan m joltc)

# Platform specific includes
IF (WIN32)
ELSE()
    target_link_libraries(SparkCore PRIVATE xcb X11 X11-xcb)
ENDIF()

# Tests
# if (SPARK_BUILD_TESTS) 
#     add_subdirectory(tests)
#     target_include_directories(spark_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
#     target_link_libraries(spark_tests PRIVATE SparkCore)
# endif()

# Create Engine
IF (NOT SPARK_CORE_ONLY)
add_subdirectory(tools/resource_copy)
target_include_directories(resource_copy PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(ASSIMP_NO_EXPORT OFF)
set(ASSIMP_BUILD_ZLIB ON)
set(ASSIMP_BUILD_TESTS OFF)
set(ASSIMP_INSTALL OFF)
set(ASSIMP_WARNINGS_AS_ERRORS OFF)
set(ASSIMP_INSTALL_PDB OFF)
set(ASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT OFF)
set(ASSIMP_BUILD_ALL_INPORTERS_BY_DEFAULT OFF)
set(ASSIMP_BUILD_GLTF_INPORTER ON)
set(ASSIMP_BUILD_GLTF2_INPORTER ON)
# target_compile_options(resource_copy PRIVATE "-g" "-O2" "-fno-lto")
target_compile_options(resource_copy PRIVATE "-fno-lto")

add_subdirectory(dependencies/assimp)
target_link_libraries(resource_copy PRIVATE assimp)
file(GLOB_RECURSE ENGINE_SOURCE_FILES 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/engine/*.c
)
add_library(SparkEngine STATIC ${ENGINE_SOURCE_FILES})
target_link_libraries(SparkEngine PUBLIC SparkCore)


# Resources 
set(SPARK_RESOURCE_DIRS ${SPARK_RESOURCE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/assets/)
foreach (dir IN ITEMS ${SPARK_RESOURCE_DIRS}) 
    file(GLOB_RECURSE DIR_RESOURCES ${dir}/*)
    foreach (file IN ITEMS ${DIR_RESOURCES}) 
        cmake_path(GET file PARENT_PATH out_path)
        cmake_path(RELATIVE_PATH out_path BASE_DIRECTORY ${dir} OUTPUT_VARIABLE out_path)
        cmake_path(GET file FILENAME file_name)
        cmake_path(GET file EXTENSION file_extension)
        cmake_path(REMOVE_EXTENSION file_name OUTPUT_VARIABLE file_name)

        if (${file_extension} STREQUAL ".glb")
            set(new_extension ".s3d")
        elseif (${file_extension} STREQUAL ".glsl")
            set(new_extension "")
        elseif (${file_extension} STREQUAL ".trs")
            set(new_extension "")
        elseif ((${file_extension} STREQUAL ".psd")   OR 
                (${file_extension} STREQUAL ".xcf")   OR 
                (${file_extension} STREQUAL ".blend") OR 
                (${file_extension} STREQUAL ".blend1"))
            continue()
        else()
            set(new_extension ${file_extension})
        endif()

        make_directory(assets/${out_path})

        set(file_target ${file}-target)
        string(REPLACE "/" "-" file_target ${file_target})

        set(file_out_path ${CMAKE_BINARY_DIR}/assets/${out_path}/${file_name}${new_extension})

        add_custom_target(${file_target} SOURCES ${file})
        add_dependencies(SparkEngine ${file_target})


        message("Adding dependency ${file_out_path}")
        add_custom_command(
            OUTPUT ${file_out_path}
            MAIN_DEPENDENCY ${file}
            COMMAND resource_copy ${file} "${CMAKE_BINARY_DIR}/assets/${out_path}" ${file_name} ${file_extension}
            COMMENT "Copying resource: ${file} -> ${file_out_path}"
            BYPRODUCTS ${file_out_path}
            DEPENDS ${file} resource_copy 
            VERBATIM
        )
    endforeach()
endforeach()
endif()
