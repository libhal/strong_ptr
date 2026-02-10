# monotonic_allocator

## monotonic_allocator\<MemorySize\>

Defined in namespace `mem`

*import strong_ptr;*

A stack-allocated monotonic memory arena for use with `std::pmr` allocators.
Allocations advance sequentially through a fixed-size buffer. If the allocator
is destroyed while allocations are still outstanding, `std::terminate` is
called.

Use `make_monotonic_allocator<N>()` to create instances.

```{doxygenstruct} v1::monotonic_allocator
```

## make_monotonic_allocator

```{doxygenfunction} v1::make_monotonic_allocator
```

## Usage Examples

### Basic allocation and deallocation

```cpp
import strong_ptr;

// Create a monotonic allocator with 32 bytes of storage
auto allocator = mem::make_monotonic_allocator<32>();

// Allocate memory using the -> operator
auto* raw = allocator->allocate(sizeof(int), alignof(int));
auto* value = static_cast<int*>(raw);
*value = 42;

// Must deallocate before the allocator is destroyed,
// otherwise std::terminate is called
allocator->deallocate(raw, sizeof(int));
```

### Using with strong_ptr

```cpp
import strong_ptr;

// Create an allocator with enough space for the object + control block
auto allocator = mem::make_monotonic_allocator<256>();

// Pass the allocator as the memory resource for strong_ptr
auto ptr = mem::make_strong_ptr<int>(allocator.resource(), 42);
// ptr is automatically deallocated when all references are released
```

### Overflow behavior

Allocating beyond the buffer size throws `std::bad_alloc`:

```cpp
import strong_ptr;

auto allocator = mem::make_monotonic_allocator<8>();

// Allocate 8 bytes (fills the buffer)
auto* p1 = allocator->allocate(sizeof(int), alignof(int));
auto* p2 = allocator->allocate(sizeof(int), alignof(int));

// This will throw std::bad_alloc — no space left
try {
  auto* p3 = allocator->allocate(sizeof(int), alignof(int));
} catch (std::bad_alloc const&) {
  // Handle out-of-memory
}

allocator->deallocate(p1, sizeof(int));
allocator->deallocate(p2, sizeof(int));
```

### Leak detection

If allocations are not fully deallocated before the allocator is destroyed,
`std::terminate` is called:

```cpp
import strong_ptr;

{
  auto allocator = mem::make_monotonic_allocator<32>();
  auto* raw = allocator->allocate(sizeof(int), alignof(int));
  // Forgetting to deallocate here will call std::terminate
  // when allocator goes out of scope!
}
```

## monotonic_allocator_base

```{doxygenstruct} v1::monotonic_allocator_base
```
