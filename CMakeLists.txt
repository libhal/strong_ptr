# Copyright 2024 - 2025 Khalil Estell and the libhal contributors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 4.0)

# Generate compile commands for anyone using our libraries.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_COLOR_DIAGNOSTICS ON)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

project(strong_ptr LANGUAGES CXX)

# Options (can be overridden by Conan or command line)
option(LIBHAL_ENABLE_CLANG_TIDY "Enable clang-tidy checks" OFF)
option(LIBHAL_CLANG_TIDY_FIX "Apply clang-tidy fixes automatically. If set to ON, will automatically enable clang-tidy." OFF)

# ==============================================================================
# Find clang-tidy
# ==============================================================================

if(CMAKE_CROSSCOMPILING)
    message(STATUS "Cross compiling, skipping clang-tidy")
elseif(LIBHAL_ENABLE_CLANG_TIDY OR LIBHAL_CLANG_TIDY_FIX)
    find_program(CLANG_TIDY_EXE NAMES clang-tidy)
    if(CLANG_TIDY_EXE)
        message(STATUS "Clang-tidy found: ${CLANG_TIDY_EXE}")
        set(CLANG_TIDY_CMD "${CLANG_TIDY_EXE};--config-file=${CMAKE_CURRENT_SOURCE_DIR}/.clang-tidy")
        if(LIBHAL_CLANG_TIDY_FIX)
            list(APPEND CLANG_TIDY_CMD "--fix")
        endif()
        set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY_CMD})
    else()
        message(STATUS "Clang-tidy not found - continuing without clang-tidy")
    endif()
else()
    message(STATUS "Clang-tidy checks disabled (use -o enable_clang_tidy=True to enable)")
endif()

# ==============================================================================
# Compile Options Interface Libraries
# ==============================================================================

# Common compile options for all targets
add_library(libhal_compile_flags INTERFACE)
target_compile_options(libhal_compile_flags INTERFACE
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:
        -g -Werror -Wall -Wextra -Wshadow -fexceptions -fno-rtti
        -Wno-unused-command-line-argument -pedantic>
    $<$<CXX_COMPILER_ID:MSVC>:
        /W4 /WX /EHsc /permissive- /GR->
)

# AddressSanitizer (GCC/Clang on non-Windows only)
add_library(libhal_asan INTERFACE)
if(NOT WIN32)
    target_compile_options(libhal_asan INTERFACE
        $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-fsanitize=address>
    )
    target_link_options(libhal_asan INTERFACE
        $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-fsanitize=address>
    )
endif()

# ==============================================================================
# Library
# ==============================================================================

add_library(strong_ptr STATIC)
add_library(strong_ptr::strong_ptr ALIAS strong_ptr)

target_compile_features(strong_ptr PUBLIC cxx_std_23)

target_sources(strong_ptr PUBLIC
    FILE_SET CXX_MODULES
    TYPE CXX_MODULES
    FILES modules/strong_ptr.cppm
)

# Use compile options directly for exported library (avoids export set issues)
target_compile_options(strong_ptr PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:
        -g -Werror -Wall -Wextra -Wshadow -fexceptions -fno-rtti
        -Wno-unused-command-line-argument>
    $<$<CXX_COMPILER_ID:MSVC>:
        /W4 /WX /EHsc /GR->
)

include(GNUInstallDirs)

install(
    TARGETS strong_ptr
    EXPORT strong_ptr_targets
    FILE_SET CXX_MODULES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    CXX_MODULES_BMI DESTINATION "${CMAKE_INSTALL_LIBDIR}/bmi"
)

install(
    EXPORT strong_ptr_targets
    FILE "strong_ptr-config.cmake"
    NAMESPACE strong_ptr::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/strong_ptr"
    CXX_MODULES_DIRECTORY "cxx-modules"
)

# ==============================================================================
# Unit testing
# ==============================================================================

if(CMAKE_CROSSCOMPILING)
    message(STATUS "Cross compiling, skipping unit test execution")
else()
    message(STATUS "Building unit tests!")

    include(CTest)
    enable_testing()

    find_package(ut REQUIRED)

    # List of test files (without .test.cpp extension)
    set(TEST_NAMES
        enable_from_this
        strong_ptr
        mixins
        weak_ptr
        optional_ptr
        monotonic_allocator
    )

    foreach(TEST_NAME IN LISTS TEST_NAMES)
        set(TEST_TARGET "test_${TEST_NAME}")
        add_executable(${TEST_TARGET})

        target_sources(${TEST_TARGET} PUBLIC
            FILE_SET CXX_MODULES
            TYPE CXX_MODULES
            FILES tests/util.cppm
            PRIVATE
            tests/${TEST_NAME}.test.cpp
        )

        target_compile_features(${TEST_TARGET} PRIVATE cxx_std_23)
        target_link_libraries(${TEST_TARGET} PRIVATE
            Boost::ut
            strong_ptr
            libhal_compile_flags
            libhal_asan
        )
        add_test(NAME ${TEST_TARGET} COMMAND ${TEST_TARGET})
    endforeach()
endif()

# Always run this custom target by making it depend on ALL
add_custom_target(copy_compile_commands ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_BINARY_DIR}/compile_commands.json
    ${CMAKE_SOURCE_DIR}/compile_commands.json
    DEPENDS ${CMAKE_BINARY_DIR}/compile_commands.json)
