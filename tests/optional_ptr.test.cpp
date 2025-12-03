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

      expect(throws<mem::nullptr_access>([&] {
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
      expect(throws<mem::nullptr_access>(
        [&] { [[maybe_unused]] auto _ = empty->value(); }))
        << "Accessing null optional with arrow operator should throw\n";
      expect(throws<mem::nullptr_access>(
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
}  // namespace mem
