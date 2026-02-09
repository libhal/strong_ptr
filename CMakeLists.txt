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

project(strong_ptr LANGUAGES CXX)

# libhal cmake utilities from libhal-cmake-util
# Source: https://github.com/libhal/libhal-cmake-util
find_package(LibhalCMakeUtil REQUIRED)

libhal_project_init()
libhal_add_library(strong_ptr MODULES modules/strong_ptr.cppm)
libhal_apply_compile_options(strong_ptr)
libhal_install_library(strong_ptr NAMESPACE libhal)
libhal_add_tests(strong_ptr
    TEST_NAMES
        enable_from_this
        strong_ptr
        mixins
        weak_ptr
        optional_ptr
        monotonic_allocator

    MODULES
        tests/util.cppm
)
