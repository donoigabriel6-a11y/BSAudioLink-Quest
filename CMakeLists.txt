# include some defines automatically made by qpm
include(qpm_defines.cmake)

cmake_minimum_required(VERSION 3.21)
project(${COMPILE_ID})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_compile_options(-frtti -fexceptions -fvisibility=hidden -O3)
add_compile_definitions(VERSION=\\"${MOD_VERSION}\\")
add_compile_definitions(MOD_ID=\\"${MOD_ID}\\")

RECURSE_FILES(cpp_file_list ${SOURCE_DIR}/*.cpp)
RECURSE_FILES(c_file_list ${SOURCE_DIR}/*.c)
RECURSE_FILES(inline_hook_c ${EXTERN_DIR}/includes/beatsaber-hook/shared/inline-hook/*.c)
RECURSE_FILES(inline_hook_cpp ${EXTERN_DIR}/includes/beatsaber-hook/shared/inline-hook/*.cpp)

add_library(${COMPILE_ID} SHARED
    ${cpp_file_list}
    ${c_file_list}
    ${inline_hook_c}
    ${inline_hook_cpp}
)

target_include_directories(${COMPILE_ID} PRIVATE ${SOURCE_DIR} ${INCLUDE_DIR} ${SHARED_DIR})
target_link_libraries(${COMPILE_ID} PRIVATE -llog -lz)
include(extern.cmake)

add_custom_command(TARGET ${COMPILE_ID} POST_BUILD
    COMMAND ${CMAKE_STRIP} -d --strip-all "lib${COMPILE_ID}.so" -o "stripped_lib${COMPILE_ID}.so")
add_custom_command(TARGET ${COMPILE_ID} POST_BUILD COMMAND ${CMAKE_COMMAND} -E make_directory debug)
add_custom_command(TARGET ${COMPILE_ID} POST_BUILD COMMAND ${CMAKE_COMMAND} -E rename lib${COMPILE_ID}.so debug/lib${COMPILE_ID}.so)
add_custom_command(TARGET ${COMPILE_ID} POST_BUILD COMMAND ${CMAKE_COMMAND} -E rename stripped_lib${COMPILE_ID}.so lib${COMPILE_ID}.so)

foreach(so_file ${so_list})
    cmake_path(GET so_file FILENAME file)
    add_custom_command(TARGET ${COMPILE_ID} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${so_file} debug/${file})
    add_custom_command(TARGET ${COMPILE_ID} POST_BUILD COMMAND ${CMAKE_STRIP} -g -S -d --strip-all ${so_file} -o ${file})
endforeach()

foreach(a_file ${a_list})
    cmake_path(GET a_file FILENAME file)
    add_custom_command(TARGET ${COMPILE_ID} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${a_file} debug/${file})
endforeach()

include(assets_include.cmake)