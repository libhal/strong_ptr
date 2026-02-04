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

namespace mem {
extern void monotonic_test();
extern void combined_mixins_test();
extern void enable_strong_from_this_test();
extern void weak_ptr_test();
extern void optional_ptr_conversion_test();
extern void optional_ptr_test();
extern void strong_ptr_test();
extern void strong_ptr_only_test();
}  // namespace mem

int main()
{
  mem::monotonic_test();
  mem::combined_mixins_test();
  mem::enable_strong_from_this_test();
  mem::weak_ptr_test();
  mem::optional_ptr_conversion_test();
  mem::optional_ptr_test();
  mem::strong_ptr_test();
  mem::strong_ptr_only_test();
}
