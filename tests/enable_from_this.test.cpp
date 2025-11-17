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
#include <memory_resource>

import strong_ptr;
import strong_ptr_unit_test;

namespace mem {
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
}  // namespace mem
