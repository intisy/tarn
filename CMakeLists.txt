cmake_minimum_required(VERSION 3.15)
project(tarn C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

function(tarn_strict_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /WX /utf-8)
        target_compile_definitions(${target} PRIVATE _CRT_SECURE_NO_WARNINGS)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
endfunction()

add_library(tarn_check_objects OBJECT
    src/boundary.c
    src/check.c
    src/directory.c
    src/generate.c
    src/ignore.c
    src/links.c
    src/memory.c
    src/org_repos.c
    src/path.c
    src/readme.c
    src/report.c
    src/source_children.c
    src/rules.c
    src/snapshot.c
    src/string_list.c
    src/text.c
    src/walk.c)
target_include_directories(tarn_check_objects PUBLIC src)
tarn_strict_warnings(tarn_check_objects)

add_executable(tarn-check src/main.c)
target_link_libraries(tarn-check PRIVATE tarn_check_objects)
tarn_strict_warnings(tarn-check)

if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    enable_testing()
    add_subdirectory(tests)
endif()
