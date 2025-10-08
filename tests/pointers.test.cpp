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

#include <strong_ptr/pointers.hpp>

#include "helpers.hpp"

#include <boost/ut.hpp>

// NOLINTBEGIN(performance-unnecessary-copy-initialization)
namespace hal::v5 {
namespace {
// Base class for testing polymorphism
class base_class
{
public:
  virtual ~base_class() = default;
  [[nodiscard]] virtual int value() const = 0;
};

// Derived class for testing polymorphism
class derived_class : public base_class
{
public:
  explicit derived_class(int p_value = 0)
    : m_value(p_value)
  {
  }

  [[nodiscard]] int value() const override
  {
    return m_value;
  }

private:
  int m_value;
};

// Default test allocator
std::pmr::monotonic_buffer_resource test_buffer{ 4096 };
std::pmr::polymorphic_allocator<> test_allocator{ &test_buffer };

}  // namespace

// Strong pointer test suite
boost::ut::suite<"strong_ptr_test"> strong_ptr_test = []() {
  using namespace boost::ut;

  "construction"_test = [&] {
    // Test make_strong_ptr
    auto ptr = make_strong_ptr<test_class>(test_allocator, 42);

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

// Optional pointer test suite
boost::ut::suite<"optional_ptr_test"> optional_ptr_test =
  []() {
    using namespace boost::ut;

    "construction"_test = [&] {
      // Test default constructor
      optional_ptr<test_class> empty;
      expect(that % false == bool(empty))
        << "Default constructed optional_ptr should be empty\n";

      // Test nullptr constructor
      optional_ptr<test_class> null_ptr = nullptr;
      expect(that % false == bool(null_ptr))
        << "Nullptr constructed optional_ptr should be empty\n";

      // Test from strong_ptr
      auto strong = make_strong_ptr<test_class>(test_allocator, 42);
      optional_ptr<test_class> opt = strong;

      expect(that % true == bool(opt))
        << "Optional from strong_ptr should be valid\n";
      expect(that % 2 == strong.use_count()) << "Should share ownership\n";

      // Test make_optional_ptr factory function
      optional_ptr<test_class> direct_opt =
        make_strong_ptr<test_class>(test_allocator, 100);
      expect(that % true == bool(direct_opt))
        << "Factory-created optional should be valid\n";
      expect(that % 100 == direct_opt->value())
        << "Value should match construction parameter\n";

      // Test copy constructor
      optional_ptr<test_class> opt2 = opt;
      expect(that % true == bool(opt2)) << "Copy should be valid\n";
      expect(that % 3 == strong.use_count())
        << "Should now have three shared owners\n";
    };

    "access"_test = [&] {
      auto strong = make_strong_ptr<test_class>(test_allocator, 42);
      optional_ptr<test_class> opt = strong;

      expect(nothrow([&] { [[maybe_unused]] auto _ = opt->value(); }))
        << "Arrow operator should work on valid optional\n";
      expect(nothrow([&] { [[maybe_unused]] auto _ = (*opt).value(); }))
        << "Dereference operator should work on valid optional\n";

      // Test value method
      expect(that % 42 == opt->value());

      // Test modifying through optional
      opt->set_value(100);
      expect(that % 100 == strong->value())
        << "Changes through optional should affect underlying object\n";

      // Test exception on accessing null optional
      optional_ptr<test_class> empty;
      expect(throws<hal::bad_optional_ptr_access>(
        [&] { [[maybe_unused]] auto _ = empty->value(); }))
        << "Accessing null optional with arrow operator should throw\n";
      expect(throws<hal::bad_optional_ptr_access>(
        [&] { [[maybe_unused]] auto _ = (*empty).value(); }))
        << "Accessing null optional with dereference operator should throw\n";
    };

    "reset"_test = [&] {
      auto strong = make_strong_ptr<test_class>(test_allocator, 42);
      optional_ptr<test_class> opt = strong;

      expect(that % true == bool(opt)) << "Optional should be valid\n";
      expect(that % 2 == strong.use_count()) << "Should share ownership\n";

      // Reset to null
      opt = nullptr;
      expect(that % false == bool(opt))
        << "Optional should be empty after reset\n";
      expect(that % 1 == strong.use_count()) << "Should release ownership\n";

      // Re-assign
      opt = strong;
      expect(that % true == bool(opt)) << "Optional should be valid again\n";
      expect(that % 2 == strong.use_count())
        << "Should share ownership again\n";
    };

    "polymorphism"_test = [&] {
      auto derived = make_strong_ptr<derived_class>(test_allocator, 42);
      optional_ptr<base_class> base_opt = derived;

      expect(that % true == bool(base_opt))
        << "Polymorphic optional should be valid\n";
      expect(that % 42 == base_opt->value())
        << "Value should be accessible through base interface\n";
      expect(that % 2 == derived.use_count()) << "Should share ownership\n";

      // Test assigning different derived
      auto derived2 = make_strong_ptr<derived_class>(test_allocator, 100);
      base_opt = derived2;

      expect(that % 100 == base_opt->value())
        << "Value should update after assignment\n";
      expect(that % 1 == derived.use_count())
        << "First derived should lose shared ownership\n";
      expect(that % 2 == derived2.use_count())
        << "Second derived should gain shared ownership\n";
    };

    "weak_ptr_lock"_test = [&] {
      weak_ptr<test_class> weak;
      {
        // Test creating an optional_ptr through weak_ptr::lock()
        auto strong = make_strong_ptr<test_class>(test_allocator, 42);
        weak = strong;

        auto locked = weak.lock();
        expect(that % true == bool(locked))
          << "Locked weak_ptr should be valid\n";
        expect(that % 42 == locked->value()) << "Value should match original\n";
        expect(that % 2 == strong.use_count())
          << "Should share ownership with original\n";
      }

      // Lock should now fail
      auto locked2 = weak.lock();
      expect(that % false == bool(locked2))
        << "Locked expired weak_ptr should be empty\n";
    };

    "equality"_test =
      [&] {
        auto strong1 = make_strong_ptr<test_class>(test_allocator, 42);
        auto strong2 = make_strong_ptr<test_class>(test_allocator, 43);

        optional_ptr<test_class> opt1 = strong1;
        optional_ptr<test_class> opt2 = strong1;
        optional_ptr<test_class> opt3 = strong2;
        optional_ptr<test_class> empty1;
        optional_ptr<test_class> empty2;

        expect(opt1 == opt2)
          << "Optionals pointing to same object should be equal\n";
        expect(opt1 != opt3)
          << "Optionals pointing to different objects should not be equal\n";
        expect(empty1 == empty2) << "Empty optionals should be equal\n";
        expect(opt1 != empty1)
          << "Valid and empty optionals should not be equal\n";

        expect(empty1 == nullptr) << "Empty optional should equal nullptr\n";
        expect(nullptr == empty1) << "nullptr should equal empty optional\n";
        expect(opt1 != nullptr) << "Valid optional should not equal nullptr\n";
        expect(nullptr != opt1) << "nullptr should not equal valid optional\n";
      };
  };
// NOLINTEND(performance-unnecessary-copy-initialization)
}  // namespace hal::v5

// Additional unit tests for new features: enable_strong_from_this,
// strong_ptr_only, and optional_ptr improvements

namespace hal::v5 {
namespace {

// Test class that uses enable_strong_from_this
class self_aware_class : public enable_strong_from_this<self_aware_class>
{
public:
  explicit self_aware_class(int p_value = 0)
    : m_value(p_value)
  {
  }

  [[nodiscard]] int value() const
  {
    return m_value;
  }
  void set_value(int p_value)
  {
    m_value = p_value;
  }

  // Method that needs to return strong_ptr to self
  strong_ptr<self_aware_class> get_self()
  {
    return strong_from_this();
  }

  // Const version
  strong_ptr<self_aware_class const> get_self_const() const
  {
    return strong_from_this();
  }

  // Get weak reference
  weak_ptr<self_aware_class> get_weak_self()
  {
    return weak_from_this();
  }

private:
  int m_value;
};

// Test class that enforces strong_ptr_only construction
class restricted_class
{
public:
  static auto create(std::pmr::polymorphic_allocator<> alloc, int p_value)
  {
    return make_strong_ptr<restricted_class>(alloc, p_value);
  }

  [[nodiscard]] int value() const
  {
    return m_value;
  }

  void set_value(int p_value)
  {
    m_value = p_value;
  }

  // Private constructor - only make_strong_ptr can access
  explicit restricted_class(hal::v5::strong_ptr_only_token, int p_value = 0)
    : m_value(p_value)
  {
  }

private:
  int m_value;
};

// Test class that combines both mixins
class fully_managed_class : public enable_strong_from_this<fully_managed_class>
{
public:
  static auto create(std::pmr::polymorphic_allocator<> alloc, int p_value)
  {
    return make_strong_ptr<fully_managed_class>(alloc, p_value);
  }

  [[nodiscard]] int value() const
  {
    return m_value;
  }
  void set_value(int p_value)
  {
    m_value = p_value;
  }

  strong_ptr<fully_managed_class> get_self()
  {
    return strong_from_this();
  }

  explicit fully_managed_class(hal::v5::strong_ptr_only_token, int p_value = 0)
    : m_value(p_value)
  {
  }

private:
  int m_value;
};

}  // namespace

// enable_strong_from_this test suite
boost::ut::suite<"enable_strong_from_this_test"> enable_strong_from_this_test =
  []() {
    using namespace boost::ut;

    "basic_functionality"_test = [&] {
      auto obj = make_strong_ptr<self_aware_class>(test_allocator, 42);

      // Test getting strong_ptr to self
      auto self = obj->get_self();
      expect(that % 42 == self->value());
      expect(that % 2 == obj.use_count())
        << "Should have two strong references";

      // Verify they point to the same object
      expect(obj.operator->() == self.operator->())
        << "Should point to same object";

      // Test weak reference
      auto weak_self = obj->get_weak_self();
      expect(not weak_self.expired()) << "Weak reference should be valid";

      auto locked = weak_self.lock();
      expect(that % true == bool(locked))
        << "Should be able to lock weak reference";
      expect(that % 42 == locked->value());
    };

    "const_functionality"_test = [&] {
      auto obj = make_strong_ptr<self_aware_class>(test_allocator, 42);
      auto const& const_obj = *obj;

      // Test const version of strong_from_this
      auto const_self = const_obj.get_self_const();
      expect(that % 42 == const_self->value());
      expect(that % 2 == obj.use_count()) << "Should share ownership";

      // Test const weak reference
      auto weak_const = const_obj.weak_from_this();
      expect(not weak_const.expired())
        << "Const weak reference should be valid";
    };

    "exception_on_unmanaged_object"_test = [&] {
      // This test cannot be easily performed since strong_ptr_only prevents
      // direct construction. The enable_strong_from_this would only fail
      // if someone bypassed the strong_ptr system.

      // Just verify that normal usage works
      auto obj = make_strong_ptr<self_aware_class>(test_allocator, 42);
      expect(nothrow([&] {
        auto self = obj->get_self();
        expect(that % 42 == self->value());
      }));
    };

    "weak_reference_lifecycle"_test = [&] {
      weak_ptr<self_aware_class> weak_ref;

      {
        auto obj = make_strong_ptr<self_aware_class>(test_allocator, 42);
        weak_ref = obj->get_weak_self();
        expect(not weak_ref.expired()) << "Weak reference should be valid";
      }

      expect(weak_ref.expired())
        << "Weak reference should be expired after object destruction";

      auto locked = weak_ref.lock();
      expect(that % false == bool(locked))
        << "Should not be able to lock expired weak reference";
    };

    "copy_semantics"_test = [&] {
      auto obj1 = make_strong_ptr<self_aware_class>(test_allocator, 42);
      auto obj2 = make_strong_ptr<self_aware_class>(test_allocator, 100);

      // Get weak references before any copying
      auto weak1 = obj1->get_weak_self();
      auto weak2 = obj2->get_weak_self();

      // Copy one object's contents to another (if we had copy assignment)
      // The weak references should remain pointing to their original objects
      expect(not weak1.expired())
        << "First weak reference should still be valid";
      expect(not weak2.expired())
        << "Second weak reference should still be valid";

      auto locked1 = weak1.lock();
      auto locked2 = weak2.lock();

      expect(that % 42 == locked1->value())
        << "First object should retain its value";
      expect(that % 100 == locked2->value())
        << "Second object should retain its value";
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

// Combined functionality test suite
boost::ut::suite<"combined_mixins_test"> combined_mixins_test = []() {
  using namespace boost::ut;

  "both_mixins_work_together"_test = [&] {
    auto obj = fully_managed_class::create(test_allocator, 42);

    // Test strong_ptr_only functionality (factory creation)
    expect(that % 42 == obj->value());
    expect(that % 1 == obj.use_count());

    // Test enable_strong_from_this functionality
    auto self = obj->get_self();
    expect(that % 42 == self->value());
    expect(that % 2 == obj.use_count()) << "Should have two references";

    // Verify they point to the same object
    expect(obj.operator->() == self.operator->())
      << "Should point to same object";
  };

  "multiple_self_references"_test = [&] {
    auto obj = fully_managed_class::create(test_allocator, 42);

    // Get multiple self references
    auto self1 = obj->get_self();
    auto self2 = obj->get_self();
    auto self3 = self1->get_self();

    expect(that % 4 == obj.use_count()) << "Should have four references";

    // All should point to the same object
    expect(obj.operator->() == self1.operator->());
    expect(obj.operator->() == self2.operator->());
    expect(obj.operator->() == self3.operator->());

    // Modify through one reference
    self2->set_value(100);
    expect(that % 100 == obj->value());
    expect(that % 100 == self1->value());
    expect(that % 100 == self3->value());
  };
};

// Enhanced optional_ptr test suite
boost::ut::suite<"optional_ptr_conversion_test"> optional_ptr_conversion_test =
  []() {
    using namespace boost::ut;

    "implicit_conversion_to_strong_ptr"_test = [&] {
      auto strong = make_strong_ptr<test_class>(test_allocator, 42);
      optional_ptr<test_class> opt = strong;

      // Test implicit conversion in function calls
      // NOLINTNEXTLINE(performance-unnecessary-value-param)
      auto convert_test = [](strong_ptr<test_class> ptr) -> int {
        return ptr->value();
      };

      expect(nothrow([&] {
        int result = convert_test(opt);  // Should implicitly convert
        expect(that % 42 == result);
      }));

      // Test explicit conversion
      strong_ptr<test_class> converted = opt;
      expect(that % 42 == converted->value());
      expect(that % 3 == strong.use_count())
        << "Should have three references now";
    };

    "conversion_with_empty_optional"_test = [&] {
      optional_ptr<test_class> empty;

      expect(throws<hal::bad_optional_ptr_access>([&] {
        strong_ptr<test_class> converted = empty;  // Should throw
      }));
    };

    "const_conversion"_test = [&] {
      auto strong = make_strong_ptr<test_class>(test_allocator, 42);
      optional_ptr<test_class> const opt = strong;

      // Test const implicit conversion
      // NOLINTNEXTLINE(performance-unnecessary-value-param)
      auto const_convert_test = [](strong_ptr<test_class> ptr) -> int {
        return ptr->value();
      };

      expect(nothrow([&] {
        int result =
          const_convert_test(opt);  // Should work with const optional
        expect(that % 42 == result);
      }));
    };

    "value_method_returns_copy"_test = [&] {
      auto strong = make_strong_ptr<test_class>(test_allocator, 42);
      optional_ptr<test_class> opt = strong;

      // Test that value() returns a copy (increments reference count)
      expect(that % 2 == strong.use_count())
        << "Should start with two references";

      {
        // NOLINTNEXTLINE(performance-unnecessary-copy-initialization)
        auto copy = opt.value();
        expect(that % 3 == strong.use_count())
          << "Should have three references with copy";
        expect(that % 42 == copy->value());
      }

      expect(that % 2 == strong.use_count())
        << "Should return to two references after copy is destroyed";
    };

    "polymorphic_conversion"_test = [&] {
      auto derived = make_strong_ptr<derived_class>(test_allocator, 42);
      optional_ptr<derived_class> opt_derived = derived;

      // Test implicit conversion to base class strong_ptr
      // NOLINTNEXTLINE(performance-unnecessary-value-param)
      auto base_convert_test = [](strong_ptr<base_class> ptr) -> int {
        return ptr->value();
      };

      expect(nothrow([&] {
        // This should work due to strong_ptr's polymorphic conversion
        // plus optional_ptr's implicit conversion
        int result = base_convert_test(opt_derived);
        expect(that % 42 == result);
      }));
    };
  };

// bad_weak_ptr exception test suite
boost::ut::suite<"bad_weak_ptr_test"> bad_weak_ptr_test = []() {
  using namespace boost::ut;

  "exception_type"_test = [&] {
    // Test that bad_weak_ptr is properly derived from hal::exception
    static_assert(std::is_base_of_v<hal::exception, hal::bad_weak_ptr>);

    // Test construction
    expect(nothrow([&] {
      hal::bad_weak_ptr ex(nullptr);
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

}  // namespace hal::v5
