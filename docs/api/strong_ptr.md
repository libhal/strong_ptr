# strong_ptr\<T\>

Defined in namespace `mem`

*import strong_ptr;*

## Usage Example

```C++
#include <cassert>
#include <memory_resource>

import strong_ptr;

struct sensor {
  int id;
  float calibration;
};

int main()
{
  // Create an allocator (or use your own pmr memory resource)
  auto allocator = mem::make_monotonic_allocator<4096>();

  // Create a strong_ptr - always valid, never null
  auto s = mem::make_strong_ptr<sensor>(allocator, 0x13, 3.14f);

  assert(s->id == 0x13);
  assert(s.use_count() == 1);

  // Share ownership
  auto s2 = s;
  assert(s.use_count() == 2);

  // Create a weak reference (does not extend lifetime)
  mem::weak_ptr<sensor> weak = s;

  // Lock the weak_ptr to get an optional_ptr
  mem::optional_ptr<sensor> maybe = weak.lock();
  if (maybe) {
    assert(maybe->id == 0x13);
  }

  return 0;
}
```

## APIs

```{doxygenfunction} v1::make_strong_ptr
```

```{doxygenclass} v1::strong_ptr
```
