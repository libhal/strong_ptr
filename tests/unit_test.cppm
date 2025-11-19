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

module;

#include <array>
#include <memory_resource>

export module strong_ptr_unit_test;

export import strong_ptr;

export namespace mem::inline v1 {
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

// Create a test class to use with smart pointers
class test_class
{
public:
  explicit test_class(int p_value = 0)
    : m_value(p_value)
  {
    ++instance_count;
  }

  ~test_class()
  {
    --instance_count;
  }

  test_class(test_class const& p_other)
  {
    m_value = p_other.m_value;
    ++instance_count;
  }

  test_class& operator=(test_class const& p_other)
  {
    if (this != &p_other) {
      m_value = p_other.m_value;
      ++instance_count;
    }
    return *this;
  }

  test_class(test_class&& p_other) noexcept
    : m_value(p_other.m_value)
  {
  }

  test_class& operator=(test_class&& p_other) noexcept
  {
    if (this != &p_other) {
      m_value = p_other.m_value;
    }
    return *this;
  }

  [[nodiscard]] int value() const
  {
    return m_value;
  }

  void set_value(int p_value)
  {
    m_value = p_value;
  }

  // Static counter for number of instances
  inline static int instance_count = 0;

private:
  int m_value;
};

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
  explicit restricted_class(mem::strong_ptr_only_token, int p_value = 0)
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

  explicit fully_managed_class(mem::strong_ptr_only_token, int p_value = 0)
    : m_value(p_value)
  {
  }

private:
  int m_value;
};

std::array<std::byte, 4096 * 16> buffer{};
std::pmr::monotonic_buffer_resource test_resource{ buffer.data(),
                                                   buffer.size() };
std::pmr::polymorphic_allocator<> test_allocator{ &test_resource };
}  // namespace mem::inline v1
