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
// NOLINTBEGIN(performance-unnecessary-copy-initialization)
// Strong pointer test suite
boost::ut::suite<"strong_ptr_test"> strong_ptr_test = []() {
  using namespace boost::ut;

  "construction"_test = [&] {
    // Test make_strong_ptr
    auto ptr = mem::make_strong_ptr<test_class>(test_allocator, 42);

    expect(that % 42 == ptr->value());
    expect(that % 1 == test_class::s_instance_count)
      << "Should have created one instance";
    expect(that % 1 == ptr.use_count())
      << "Should have exactly one reference\n";

    // Test copy constructor
    auto ptr2 = ptr;
    expect(that % 2 == ptr.use_count())
      << "Should have two references after copy\n";
    expect(that % 2 == ptr2.use_count())
      << "Copy should share reference count\n";

    // Test copy constructor and reference counting
    {
      auto ptr3 = ptr;
      expect(that % 3 == ptr.use_count()) << "Should have three references\n";
    }
    expect(that % 2 == ptr.use_count())
      << "Should have two references after ptr3 goes out of scope\n";

    auto ptr4 = ptr2;
    expect(that % 3 == ptr.use_count());
    expect(that % 3 == ptr4.use_count());
    expect(nothrow([&] { [[maybe_unused]] auto _ = ptr4->value(); }));
  };

  "operator overloads"_test = [&] {
    auto ptr = make_strong_ptr<test_class>(test_allocator, 42);

    // Test dereference operator
    expect(that % 42 == (*ptr).value());

    // Test arrow operator
    expect(that % 42 == ptr->value());

    // Test modifying the underlying object
    ptr->set_value(100);
    expect(that % 100 == ptr->value());
  };

  "polymorphism"_test = [&] {
    // Test polymorphic behavior
    auto derived = make_strong_ptr<derived_class>(test_allocator, 42);
    strong_ptr<base_class> base = derived;

    expect(that % 42 == base->value());
    expect(that % 2 == derived.use_count())
      << "Base and derived should share ownership\n";

    // Modify through derived
    auto derived2 = make_strong_ptr<derived_class>(test_allocator, 100);
    base = derived2;

    expect(that % 100 == base->value());
    expect(that % 1 == derived.use_count())
      << "Original derived should now have only one reference\n";
    expect(that % 2 == derived2.use_count())
      << "New derived should share with base\n";
  };

  "aliasing"_test = [&] {
    // Create a class with internal structure
    struct outer_class
    {
      int member_offset{};
      test_class inner;
      std::array<test_class, 2> array_inner;
      explicit outer_class(int p_value)
        : inner(p_value)
        , array_inner{ test_class{ p_value }, test_class{ p_value } }
      {
      }
    };

    auto outer = make_strong_ptr<outer_class>(test_allocator, 42);
    // Create an alias to the inner object
    strong_ptr<test_class> inner(outer, &outer_class::inner);

#if 0  // set to 1 to test aliasing constructor failing on random void*
    int wrong = 5;
    // ❌ Will emit an error when attempting to use this constructor.
    strong_ptr<int> inner2(outer, &wrong);
#endif

    // Create an alias to the array_inner object's index 1
    strong_ptr<test_class> array_inner(outer, &outer_class::array_inner, 1);

    expect(that % 42 == inner->value());
    expect(that % 42 == array_inner->value());

    expect(that % 3 == outer.use_count())
      << "Outer and inner alias should share ownership\n";
    expect(that % 3 == outer.use_count())
      << "Outer and inner alias should share ownership\n";

    // Modify through the alias
    inner->set_value(100);
    // Modify through the alias
    array_inner->set_value(120);

    expect(that % 100 == outer->inner.value());
    expect(that % 120 == outer->array_inner[1].value());
  };

  "alias outliving original"_test = [&] {
    // Create a class with internal structure
    struct outer_class
    {
      int member_offset{};
      test_class inner;
      std::array<test_class, 2> array_inner;
      explicit outer_class(int p_value)
        : inner(p_value)
        , array_inner{ test_class{ p_value }, test_class{ p_value } }
      {
      }
    };

    auto outlived = [] -> auto {
      auto outer = make_strong_ptr<outer_class>(test_allocator, 42);
      // Create an alias to the inner object
      strong_ptr<test_class> inner(outer, &outer_class::inner);
      // Create an alias to the array_inner object's index 1
      strong_ptr<test_class> array_inner(outer, &outer_class::array_inner, 1);

      expect(that % 42 == inner->value());
      expect(that % 42 == array_inner->value());

      expect(that % 3 == outer.use_count())
        << "Outer and inner alias should share ownership\n";
      expect(that % 3 == outer.use_count())
        << "Outer and inner alias should share ownership\n";

      // Modify through the alias
      inner->set_value(100);
      // Modify through the alias
      array_inner->set_value(120);

      expect(that % 100 == outer->inner.value());
      expect(that % 120 == outer->array_inner[1].value());

      // NOLINTNEXTLINE(performance-move-const-arg)
      auto move_becomes_copy = std::move(inner);

      // Test that explicit move becomes a copy
      expect(that % 4 == move_becomes_copy.use_count())
        << "Outer and inner alias should share ownership\n";

      return array_inner;
    }();

    expect(that % 120 == outlived->value());

    expect(that % 1 == outlived.use_count())
      << "outlived alias should have sole ownership\n";
  };

  "equality"_test = [&] {
    auto ptr1 = make_strong_ptr<test_class>(test_allocator, 42);
    auto ptr2 = ptr1;
    auto ptr3 = make_strong_ptr<test_class>(test_allocator, 43);

    expect(ptr1 == ptr2) << "Copies should be equal\n";
    expect(ptr1 != ptr3) << "Different objects should not be equal\n";
  };

  "destruction"_test = [&] {
    expect(that % 0 == test_class::s_instance_count)
      << "Should start with no instances\n";

    {
      auto ptr = make_strong_ptr<test_class>(test_allocator, 42);
      expect(that % 1 == test_class::s_instance_count)
        << "Should have one instance\n";
    }

    expect(that % 0 == test_class::s_instance_count)
      << "Instance should be destroyed\n";
  };
};

// strong_ptr_only test suite
boost::ut::suite<"strong_ptr_only_test"> strong_ptr_only_test = []() {
  using namespace boost::ut;

  "factory_creation"_test = [&] {
    auto obj = restricted_class::create(test_allocator, 42);
    expect(that % 42 == obj->value());
    expect(that % 1 == obj.use_count()) << "Should have one reference";
  };

  "copy_move_prevention"_test = [&] {
    auto obj = restricted_class::create(test_allocator, 42);

    // These should not compile if uncommented:
    // auto copy = *obj;                    // Copy constructor deleted
    // auto moved = std::move(*obj);        // Move constructor deleted
    // restricted_class another = *obj;     // Copy constructor deleted

    // Verify the object works normally
    expect(that % 42 == obj->value());
    obj->set_value(100);
    expect(that % 100 == obj->value());
  };

  "polymorphism_with_restriction"_test = [&] {
    // Create restricted object
    auto obj = restricted_class::create(test_allocator, 42);

    // strong_ptr operations should work normally
    // NOLINTBEGIN(performance-unnecessary-copy-initialization)
    auto copy_ptr = obj;
    // NOLINTEND(performance-unnecessary-copy-initialization)
    expect(that % 2 == obj.use_count()) << "Should share ownership";
    expect(that % 42 == copy_ptr->value());
  };
};

boost::ut::suite<"monotonic_allocator_test"> monotonic_allocator_test = []() {
  using namespace boost::ut;
  "test1"_test = [&] {
    // make tests here
    monotonic_allocator<64> allocator;

    char* char_test = allocator.allocate(sizeof(char), alignof(char));
    *char_test = 'a';

    expect(that % 'a' == char_test);
    << "Assignment failed.\n";
  };
};
}  // namespace mem
