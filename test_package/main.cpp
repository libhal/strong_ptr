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

#include <cassert>

#include <memory_resource>

import strong_ptr;

int main()
{
  auto ptr = mem::make_strong_ptr<int>(std::pmr::new_delete_resource(), 42);
  assert(*ptr == 42);
  assert(ptr.use_count() == 1);
  {
    mem::optional_ptr<int> ptr2 = ptr;
    assert(*ptr2 == 42);
    *ptr2 = 55;
    assert(*ptr2 == 55);
    assert(ptr.use_count() == 2);
    assert(ptr2.use_count() == 2);
  }

  assert(ptr.use_count() == 1);
  assert(*ptr == 55);
  return 0;
}
