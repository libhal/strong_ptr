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
}  // namespace mem
