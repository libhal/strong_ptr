# strong_ptr

[![✅ CI](https://github.com/libhal/strong_ptr/actions/workflows/ci.yml/badge.svg)](https://github.com/libhal/strong_ptr/actions/workflows/ci.yml)
[![Standard](https://img.shields.io/badge/C%2B%2B-23-C%2B%2B23?logo=c%2B%2B&color=00599C&style=flat)](https://isocpp.org/std/the-standard)
[![GitHub stars](https://img.shields.io/github/stars/libhal/strong_ptr.svg)](https://github.com/libhal/strong_ptr/stargazers)
[![GitHub forks](https://img.shields.io/github/forks/libhal/strong_ptr.svg)](https://github.com/libhal/strong_ptr/network)
[![GitHub issues](https://img.shields.io/github/issues/libhal/strong_ptr.svg)](https://github.com/libhal/strong_ptr/issues)

A non-null, reference-counted smart pointer for memory-constrained systems.
Built as a safer alternative to `std::shared_ptr` with fewer foot-guns, explicit
allocator control via `std::pmr::memory_resource`, and C++23 module support.

> [!CAUTION]
>
> The APIs of this library are not stable and may change at any time before
> 1.0.0 release.

```C++
import strong_ptr;

#include <cassert>
#include <memory_resource>

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

## Why not `std::shared_ptr`?

| Problem with `std::shared_ptr`                                                | `strong_ptr` solution                                                                         |
| ----------------------------------------------------------------------------- | --------------------------------------------------------------------------------------------- |
| Can be null — dereferencing a null `shared_ptr` is UB                         | **Cannot be null** — `strong_ptr` has no default or `nullptr` constructor                     |
| Aliasing constructor accepts arbitrary `void*` — UB if lifetime doesn't match | **Safe aliasing** via pointer-to-member only, with bounds-checked array access                |
| Implicitly uses global heap (`new`/`delete`)                                  | **Explicit allocator** — requires a `std::pmr::memory_resource*` at construction              |
| Move leaves the source in a null state — use-after-move is UB                 | **Move acts as copy** — source remains valid after move, preventing use-after-move bugs       |
| No mechanism to restrict construction to the factory function                 | **`strong_ptr_only_token`** — classes can require construction through `make_strong_ptr` only |

## Key Types

### `mem::strong_ptr<T>`

A non-null, reference-counted shared pointer. Must be created via
`make_strong_ptr` or from a static object using `unsafe_assume_static_tag`.

### `mem::weak_ptr<T>`

A non-owning reference that doesn't prevent destruction. Use `.lock()` to
obtain an `optional_ptr` if the object is still alive.

### `mem::optional_ptr<T>`

A nullable smart pointer — the only way to represent "no value" in this
library. Implicitly converts to `strong_ptr<T>` (throws `mem::nullptr_access`
if empty).

### `mem::monotonic_allocator<N>`

A stack-allocated bump allocator implementing `std::pmr::memory_resource`.
Calls `std::terminate` on destruction if any allocations are still outstanding,
preventing dangling references.

### `mem::enable_strong_from_this<T>`

A CRTP mixin that allows an object managed by `strong_ptr` to obtain a
`strong_ptr` or `weak_ptr` to itself:

```C++
class my_driver : public mem::enable_strong_from_this<my_driver> {
public:
  void register_callback() {
    auto self = strong_from_this();
    event_system.on_event([self]() { self->handle(); });
  }
};
```

### `mem::strong_ptr_only_token`

A construction token that restricts a class to only be constructible via
`make_strong_ptr`:

```C++
class restricted {
public:
  restricted(mem::strong_ptr_only_token, int value);
};

// Only way to create it:
auto r = mem::make_strong_ptr<restricted>(allocator, 42);
```

## Requirements

- C++23 compiler with module support
- GCC 14+, Clang 19+, or MSVC 14.34+ (Visual Studio 17.4+)

> [!WARNING]
> GCC support is not recommended. At least not with GCC 14. We plan to test with
> GCC 15 to see if module support is better.

## Adding `strong_ptr` to your project

Add the following to your `requirements()` method in your `ConanFile` class:

```python
def requirements(self):
    self.requires("strong_ptr/[^1.0.0]")
```

Replace the version with whatever is appropriate for your application. If you
don't know what version to use, consider using the
[latest](https://github.com/libhal/strong_ptr/releases/latest) release.

## Building & Installing the Library Package

Before getting started, if you haven't used libhal before, follow the
[Getting Started](https://libhal.github.io/latest/getting_started/) guide.

To build and install the package to your local conan cache:

```bash
conan create . --version=latest
```

## Contributing

See [`CONTRIBUTING.md`](CONTRIBUTING.md) for details.

## Future Considerations

- **Fuzz testing** — Add libFuzzer or AFL-based fuzz tests targeting the
  allocator and reference counting paths to catch edge cases unit tests miss.
- **Benchmarks** — Add performance benchmarks comparing `strong_ptr` against
  `std::shared_ptr` to track overhead and prevent regressions.
- **UBSan CI job** — Add UndefinedBehaviorSanitizer to CI alongside the
  existing ASan coverage.

## License

Apache License 2.0 - See [LICENSE](LICENSE) for details.

Copyright 2024 - 2025 Khalil Estell and the libhal contributors
