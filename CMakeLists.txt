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

cmake_minimum_required(VERSION 3.30)

# Generate compile commands for anyone using our libraries.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(strong_ptr LANGUAGES CXX)

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
    find_package(ut REQUIRED)
    add_executable(strong_ptr_unit_test)
    target_sources(strong_ptr_unit_test PUBLIC
        FILE_SET CXX_MODULES
        TYPE CXX_MODULES
        FILES
        tests/unit_test.cppm
        PRIVATE
        tests/main.test.cpp
        tests/optional_ptr.test.cpp
        tests/enable_from_this.test.cpp
        tests/strong_ptr.test.cpp
        tests/weak_ptr.test.cpp
        tests/mixins.test.cpp
    )
    target_compile_features(strong_ptr_unit_test PUBLIC cxx_std_23)
    target_compile_options(strong_ptr_unit_test PUBLIC
        -g
        -Werror
        -Wall
        -Wextra
        -Wshadow
        -pedantic
        -fsanitize=address
    )
    target_link_options(strong_ptr_unit_test PRIVATE -fsanitize=address)
    target_link_libraries(strong_ptr_unit_test PRIVATE Boost::ut strong_ptr)

    message(STATUS "Executing unit tests!")
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
