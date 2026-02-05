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

# ==============================================================================
# Find clang-tidy
# ==============================================================================

if(CMAKE_CROSSCOMPILING)
    message(STATUS "Cross compiling, skipping clang-tidy")
elseif(LIBHAL_ENABLE_CLANG_TIDY)
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
# Library
# ==============================================================================

add_library(strong_ptr STATIC)

target_compile_features(strong_ptr PUBLIC cxx_std_23)

target_sources(strong_ptr PUBLIC
    FILE_SET CXX_MODULES
    TYPE CXX_MODULES
    FILES modules/strong_ptr.cppm
)

target_compile_options(strong_ptr PRIVATE
    -g
    -Werror
    -Wno-unused-command-line-argument
    -Wall
    -Wextra
    -Wshadow
    -fexceptions
    -fno-rtti)

install(
    TARGETS strong_ptr
    EXPORT strong_ptr_targets
    FILE_SET CXX_MODULES DESTINATION "."
    LIBRARY DESTINATION "lib"
    ARCHIVE DESTINATION "lib"
    CXX_MODULES_BMI DESTINATION "bmi"
)

install(
    EXPORT strong_ptr_targets
    FILE "strong_ptr-config.cmake"
    DESTINATION "lib/cmake"
    CXX_MODULES_DIRECTORY "cxx-modules"
)

# ==============================================================================
# Unit testing
# ==============================================================================

if(CMAKE_CROSSCOMPILING)
    message(STATUS "Cross compiling, skipping unit test execution")
else()
    message(STATUS "Building unit tests!")

    find_package(ut REQUIRED)
    add_executable(strong_ptr_unit_test)
    target_sources(strong_ptr_unit_test PUBLIC
        FILE_SET CXX_MODULES
        TYPE CXX_MODULES
        FILES
        tests/unit_test.cppm
        PRIVATE
        tests/main.test.cpp
        tests/strong_ptr.test.cpp
        tests/monotonic_allocator.test.cpp
        tests/optional_ptr.test.cpp
        tests/weak_ptr.test.cpp
        tests/enable_from_this.test.cpp
        tests/mixins.test.cpp
    )
    target_compile_features(strong_ptr_unit_test PUBLIC cxx_std_23)
    target_compile_options(strong_ptr_unit_test PUBLIC
        -g
        -Werror
        -Wno-unused-command-line-argument
        -Wall
        -Wextra
        -Wshadow
        -pedantic
        -fexceptions
        -fno-rtti
    )

    # Enable ASAN only on non-Windows platforms (Windows doesn't support it)
    if(NOT WIN32)
        target_compile_options(strong_ptr_unit_test PUBLIC -fsanitize=address)
        target_link_options(strong_ptr_unit_test PRIVATE -fsanitize=address)
    endif()

    target_link_libraries(strong_ptr_unit_test PRIVATE Boost::ut strong_ptr)

    add_custom_target(run_tests ALL
      DEPENDS strong_ptr_unit_test
      COMMAND strong_ptr_unit_test
    )
endif()

# Always run this custom target by making it depend on ALL
add_custom_target(copy_compile_commands ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_BINARY_DIR}/compile_commands.json
    ${CMAKE_SOURCE_DIR}/compile_commands.json
    DEPENDS ${CMAKE_BINARY_DIR}/compile_commands.json)
