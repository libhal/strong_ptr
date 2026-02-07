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

#include <boost/ut.hpp>

import test_util;
import strong_ptr;

using namespace boost::ut;
using namespace mem;

int main()
{
  // NOLINTBEGIN(performance-unnecessary-copy-initialization)
  "strong_ptr::construction"_test = [&] {
    // Test make_strong_ptr
    auto ptr = mem::make_strong_ptr<test_class>(test_allocator, 42);

    expect(that % 42 == ptr->value());
    expect(that % 1 == test_class::instance_count)
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

  "strong_ptr operator overloads"_test = [&] {
    auto ptr = make_strong_ptr<test_class>(test_allocator, 42);

    // Test dereference operator
    expect(that % 42 == (*ptr).value());

    // Test arrow operator
    expect(that % 42 == ptr->value());

    // Test modifying the underlying object
    ptr->set_value(100);
    expect(that % 100 == ptr->value());
  };

  "strong_ptr polymorphism"_test = [&] {
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

  "strong_ptr::destruction"_test = [&] {
    expect(that % 0 == test_class::instance_count)
      << "Should start with no instances\n";

    {
      auto ptr = make_strong_ptr<test_class>(test_allocator, 42);
      expect(that % 1 == test_class::instance_count)
        << "Should have one instance\n";
    }

    expect(that % 0 == test_class::instance_count)
      << "Instance should be destroyed\n";
  };

  "strong_ptr_only_test::factory_creation"_test = [&] {
    auto obj = restricted_class::create(test_allocator, 42);
    expect(that % 42 == obj->value());
    expect(that % 1 == obj.use_count()) << "Should have one reference";
  };

  "strong_ptr_only_test::copy_move_prevention"_test = [&] {
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

  "statically_allocate_strong_ptr"_test = [&] {
    // Create a static object
    static int static_obj = 42;

    // Create strong_ptr to static object using unsafe_assume_static_tag
    strong_ptr<int> ptr(mem::unsafe_assume_static_tag{}, static_obj);

    // Verify the pointer works correctly
    expect(that % 42 == *ptr);

    // use_count should be 0 for statically allocated objects
    expect(that % 0 == ptr.use_count())
      << "Static strong_ptr should have use_count of 0";

    // Modify through the pointer
    *ptr = 100;
    expect(that % 100 == *ptr);
    expect(that % 100 == static_obj)
      << "Static int should be modified through strong_ptr";

    // Test copying a static strong_ptr
    auto ptr_copy = ptr;
    expect(that % 0 == ptr_copy.use_count())
      << "Copy of static strong_ptr should also have use_count of 0";
    expect(that % 100 == *ptr_copy);

    // Test copying a static strong_ptr
    mem::optional_ptr<int> opt_ptr = ptr;
    *opt_ptr = 17;
    expect(that % 0 == ptr_copy.use_count())
      << "Copy of static optional_ptr should also have use_count of 0";
    expect(that % 17 == *ptr_copy);
    expect(that % 17 == static_obj)
      << "Static int should be modified through optional_ptr";
  };

  "static_strong_ptr_no_destructor_call"_test = [&] {
    // Use a separate class to avoid affecting test_class::s_instance_count
    static bool destructor_called = false;
    struct static_tracked_class
    {
      int value;
      explicit static_tracked_class(int v)
        : value(v)
      {
      }
      ~static_tracked_class()
      {
        destructor_called = true;
      }
    };

    static static_tracked_class static_obj(999);
    destructor_called = false;  // Reset flag

    {
      // Create strong_ptr to static object using unsafe_assume_static_tag
      strong_ptr<static_tracked_class> ptr(mem::unsafe_assume_static_tag{},
                                           static_obj);

      expect(that % 999 == ptr->value);
      expect(that % 0 == ptr.use_count())
        << "Static strong_ptr should have use_count of 0";

      // Copy the pointer to ensure copies also don't call destructor
      auto ptr_copy = ptr;
      expect(that % 0 == ptr_copy.use_count());
    }

    // After scope exit, destructor should NOT have been called
    expect(not destructor_called)
      << "Static object's destructor should not be called when strong_ptr "
         "goes out of scope";

    // Verify object is still valid
    expect(that % 999 == static_obj.value)
      << "Static object should still be accessible after strong_ptr "
         "destruction";
  };

  "get_allocator_for_dynamic_allocation"_test = [&] {
    // Create a dynamically allocated strong_ptr
    auto ptr = make_strong_ptr<test_class>(test_allocator, 42);

    // Get the allocator
    auto* allocator = ptr.get_allocator();

    // Verify the allocator matches the one used for creation
    expect(that % test_allocator == allocator)
      << "Allocator should match the one used for creation";

    // Verify it's not nullptr for dynamically allocated objects
    expect(allocator != nullptr)
      << "Allocator should not be nullptr for dynamically allocated objects";

    // Test that copies return the same allocator
    auto ptr_copy = ptr;
    expect(that % test_allocator == ptr_copy.get_allocator())
      << "Copied strong_ptr should return the same allocator";
  };

  "get_allocator_for_static_allocation"_test = [&] {
    // Create a static object (using int to avoid affecting test_class instance
    // count)
    static int static_obj = 777;

    // Create strong_ptr to static object using unsafe_assume_static_tag
    strong_ptr<int> ptr(mem::unsafe_assume_static_tag{}, static_obj);

    // Get the allocator
    auto* allocator = ptr.get_allocator();

    // Verify the allocator is nullptr for statically allocated objects
    expect(allocator == nullptr)
      << "Allocator should be nullptr for statically allocated objects";

    // Test that copies also return nullptr
    auto ptr_copy = ptr;
    expect(ptr_copy.get_allocator() == nullptr)
      << "Copied static strong_ptr should also return nullptr allocator";
  };

  "get_allocator_for_aliased_ptr"_test = [&] {
    // Create a class with internal structure
    struct outer_class
    {
      test_class inner;
      std::array<test_class, 2> array_inner;
      explicit outer_class(int p_value)
        : inner(p_value)
        , array_inner{ test_class{ p_value }, test_class{ p_value } }
      {
      }
    };

    // Create the outer object with a specific allocator
    auto outer = make_strong_ptr<outer_class>(test_allocator, 42);

    // Create aliases to inner members
    strong_ptr<test_class> inner_alias(outer, &outer_class::inner);
    strong_ptr<test_class> array_alias(outer, &outer_class::array_inner, 0);

    // Verify all aliases return the same allocator as the parent
    expect(that % test_allocator == outer.get_allocator())
      << "Outer strong_ptr should return the original allocator";

    expect(that % test_allocator == inner_alias.get_allocator())
      << "Inner alias should return the same allocator as parent";

    expect(that % test_allocator == array_alias.get_allocator())
      << "Array alias should return the same allocator as parent";

    // Verify they all return the same allocator instance
    expect(outer.get_allocator() == inner_alias.get_allocator())
      << "Outer and inner alias should share the same allocator";

    expect(outer.get_allocator() == array_alias.get_allocator())
      << "Outer and array alias should share the same allocator";
  };

  "get_allocator_for_polymorphic_ptr"_test = [&] {
    // Create a derived class object
    auto derived = make_strong_ptr<derived_class>(test_allocator, 99);

    // Convert to base class pointer
    strong_ptr<base_class> base = derived;

    // Both should return the same allocator
    expect(that % test_allocator == derived.get_allocator())
      << "Derived strong_ptr should return the original allocator";

    expect(that % test_allocator == base.get_allocator())
      << "Base strong_ptr should return the same allocator";

    expect(derived.get_allocator() == base.get_allocator())
      << "Derived and base pointers should share the same allocator";
  };
  // NOLINTEND(performance-unnecessary-copy-initialization)
}
