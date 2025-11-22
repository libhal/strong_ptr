#!/usr/bin/python
#
# Copyright 2024 - 2025 Khalil Estell and the libhal contributors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from conan import ConanFile
from conan.tools.build import cross_building
from conan.tools.cmake import CMake, cmake_layout, CMakeToolchain, CMakeDeps
from pathlib import Path


class TestPackageConan(ConanFile):
    settings = "os", "arch", "compiler", "build_type"
    generators = "VirtualRunEnv"

    def build_requirements(self):
        self.tool_requires("cmake-modules-toolchain/1.0.1")
        self.tool_requires("cmake/4.1.1")
        self.tool_requires("ninja/1.13.1")

    def requirements(self):
        self.requires(self.tested_reference_str)

    def layout(self):
        cmake_layout(self)

    def inject_linker_flags(self, flags: str,  tc: CMakeToolchain) -> str:
        link_flags = tc.variables.get("CMAKE_EXE_LINKER_FLAGS", "")
        link_flags = link_flags + f" {flags}"
        link_flags = link_flags.strip()
        tc.variables["CMAKE_EXE_LINKER_FLAGS"] = link_flags
        return tc.variables["CMAKE_EXE_LINKER_FLAGS"]

    def _add_arm_specs_if_applicable(self, tc: CMakeToolchain):
        should_add_flags = (self.settings.os == "baremetal"
                            and self.settings.compiler == "gcc"
                            and str(self.settings.arch).startswith("cortex-m"))
        if not should_add_flags:
            return

        LIB_C_FLAGS = "--specs=nano.specs --specs=nosys.specs -mthumb"
        self.output.info(f"Baremetal ARM GCC Profile detected!")
        self.output.info(f'💉 injecting flags >> "{LIB_C_FLAGS}"')
        result = self.inject_linker_flags(LIB_C_FLAGS, tc)
        self.output.info(f'tc.var["CMAKE_EXE_LINKER_FLAGS"] = "{result}"')

    def generate(self):
        tc = CMakeToolchain(self)
        tc.generator = "Ninja"
        self._add_arm_specs_if_applicable(tc)
        tc.generate()

        deps = CMakeDeps(self)
        deps.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def test(self):
        if not cross_building(self):
            bin_path = Path(self.cpp.build.bindirs[0]) / "test_package"
            self.run(bin_path.absolute(), env="conanrun")
