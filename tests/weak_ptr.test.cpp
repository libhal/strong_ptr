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

namespace mem {

// Weak pointer test suite
boost::ut::suite<"weak_ptr_test"> weak_ptr_test = []() {
  using namespace boost::ut;

  "construction"_test = [&] {
    // Test creating a weak_ptr from a strong_ptr
    auto strong = make_strong_ptr<test_class>(test_allocator, 42);
    weak_ptr<test_class> weak = strong;

    expect(not weak.expired()) << "Weak pointer should not be expired\n";
    expect(that % 1 == strong.use_count())
      << "Weak reference shouldn't affect strong count\n";

    // Test default constructor
    weak_ptr<test_class> empty;
    expect(empty.expired())
      << "Default constructed weak_ptr should be expired\n";
    expect(that % 0 == empty.use_count());

    // Test copy constructor
    weak_ptr<test_class> weak2 = weak;
    expect(not weak2.expired()) << "Copied weak_ptr should not be expired\n";

    // Test from expired strong_ptr
    {
      auto temp_strong = make_strong_ptr<test_class>(test_allocator, 100);
      weak_ptr<test_class> temp_weak = temp_strong;

      expect(not temp_weak.expired()) << "Weak pointer should not be expired\n";

      // Temp_strong will go out of scope here
    }

    // Previous strong_ptr should be destroyed, but the weak_ptr should still
    // exist
    expect(that % 1 == test_class::s_instance_count)
      << "Only one test_class should exist\n";
  };

  "lock"_test = [&] {
    auto strong = make_strong_ptr<test_class>(test_allocator, 42);
    weak_ptr<test_class> weak = strong;

    // Test locking to get a strong_ptr
    {
      auto locked = weak.lock();
      expect(that % true == static_cast<bool>(locked))
        << "Lock should succeed on valid weak_ptr\n";
      expect(that % 42 == locked->value())
        << "Locked value should match original\n";
      expect(that % 2 == strong.use_count())
        << "Should now have two strong references\n";
    }

    expect(that % 1 == strong.use_count())
      << "Back to one reference after locked ptr is destroyed\n";

    // Test locking an expired weak_ptr
    weak_ptr<test_class> temp_weak;

    {
      auto temp_strong = make_strong_ptr<test_class>(test_allocator, 100);
      temp_weak = temp_strong;

      auto locked = temp_weak.lock();
      expect(that % true == bool(locked))
        << "Lock should succeed on existing weak_ptr\n";
    }

    auto locked = temp_weak.lock();
    expect(that % false == bool(locked))
      << "Lock should fail on expired weak_ptr\n";
  };

  "polymorphism"_test = [&] {
    auto derived = make_strong_ptr<derived_class>(test_allocator, 42);
    weak_ptr<base_class> base_weak = derived;

    expect(not base_weak.expired())
      << "Polymorphic weak_ptr should not be expired\n";

    auto locked = base_weak.lock();
    expect(that % true == bool(locked))
      << "Should be able to lock polymorphic weak_ptr\n";
    expect(that % 42 == locked->value())
      << "Locked value should match original\n";
  };

  "expired"_test =
    [&] {
      // Create a scope for the strong_ptr
      weak_ptr<test_class> weak;

      {
        auto strong = make_strong_ptr<test_class>(test_allocator, 42);
        weak = strong;
        expect(not weak.expired()) << "Weak pointer should not be expired\n";
      }

      expect(weak.expired())
        << "Weak pointer should be expired after strong_ptr is destroyed\n";
      expect(that % 0 == test_class::s_instance_count)
        << "Object should be destroyed\n";

      auto locked = weak.lock();
      expect(that % false == bool(locked))
        << "Locking expired weak_ptr should return null optional\n";
    };
};

// bad_weak_ptr exception test suite
boost::ut::suite<"bad_weak_ptr_test"> bad_weak_ptr_test = []() {
  using namespace boost::ut;

  "exception_type"_test = [&] {
    // Test that bad_weak_ptr is properly derived from mem::exception
    static_assert(std::is_base_of_v<mem::exception, mem::bad_weak_ptr>);

    // Test construction
    expect(nothrow([&] {
      mem::bad_weak_ptr ex(nullptr);
      // Should construct without throwing
    }));
  };

  "thrown_from_enable_strong_from_this"_test = [&] {
    // This test would require creating an unmanaged object, which is
    // prevented by strong_ptr_only. The exception handling is tested
    // indirectly through the normal usage patterns.

    // Just verify that normal usage doesn't throw
    auto obj = make_strong_ptr<self_aware_class>(test_allocator, 42);
    expect(nothrow([&] {
      auto self = obj->strong_from_this();
      expect(that % 42 == self->value());
    }));
  };
};
}  // namespace mem
