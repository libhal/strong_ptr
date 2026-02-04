// Copyright 2024 - 2025 Khalil Estell and the libhal contributors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <memory_resource>

#include <boost/ut.hpp>

import strong_ptr;
import strong_ptr_unit_test;

// NOLINTBEGIN(performance-unnecessary-copy-initialization)
namespace mem {
// Strong pointer test suite
void monotonic_test()
{
  using namespace boost::ut;
  "assignment_test"_test = [&] {
    auto allocator = mem::make_monotonic_allocator<32>();

    auto ptr1 = allocator->allocate(sizeof(char), alignof(char));
    auto char_ptr = static_cast<char*>(ptr1);
    *char_ptr = 'a';

    auto ptr2 =
      allocator->allocate(sizeof(std::uint32_t), alignof(std::uint32_t));
    auto int_ptr = static_cast<int*>(ptr2);
    *int_ptr = 1;

    expect(that % 1 == *int_ptr) << "Int assignment failed.\n";
    expect(that % 'a' == *char_ptr) << "char assignment failed.\n";
    allocator->deallocate(ptr2, sizeof(std::uint32_t));
    allocator->deallocate(ptr1, sizeof(char));
  };

// NOTE: Abort testing does not work on Windows
#if not defined(_WIN32) and not defined(_WIN64)
  "termination_test"_test = [&] {
    expect(aborts([] {
      auto allocator = mem::make_monotonic_allocator<32>();
      [[maybe_unused]] auto ptr =
        allocator->allocate(sizeof(std::uint32_t), alignof(std::uint32_t));
    }))
      << "std::terminate not called.\n";
  };
#endif

  "max_buffer_test"_test = [&] {
    auto allocator = mem::make_monotonic_allocator<8>();
    auto ptr1 =
      allocator->allocate(sizeof(std::uint32_t), alignof(std::uint32_t));
    auto int_ptr1 = static_cast<int*>(ptr1);
    *int_ptr1 = 1;

    auto ptr2 =
      allocator->allocate(sizeof(std::uint32_t), alignof(std::uint32_t));
    auto int_ptr2 = static_cast<int*>(ptr2);
    *int_ptr2 = 2;

    expect(that % 1 == *int_ptr1) << "Int assignment failed.\n";
    expect(that % 2 == *int_ptr2) << "Int assignment failed.\n";

    expect(throws<std::bad_alloc>([&] {
      [[maybe_unused]] auto ptr3 =
        allocator->allocate(sizeof(std::uint32_t), alignof(std::uint32_t));
    }))
      << "Exception not thrown when bad alloc happens.\n";

    allocator->deallocate(ptr1, sizeof(std::uint32_t));
    allocator->deallocate(ptr2, sizeof(std::uint32_t));
  };
}
}  // namespace mem
// NOLINTEND(performance-unnecessary-copy-initialization)