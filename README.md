Single producer single consumer queue implementation.
========================================================================

This is my toy implementation of a SPSC Queue as a template library. It supports concurrent enquing and dequing elements by two threads.

To compile, test and benchmark it requires CMake 3.15+ and clang or gcc with c++23 support.

You can do it by executing:
```
cmake -Bbuild -DCMAKE_BUILD_TYPE=Release
cmake --build build/
cd build
ctest
./benchmarks/benchmarks
```

It will pull main branch of [google test](https://github.com/google/googletest.git) and [google benchmark](https://github.com/google/benchmark.git). Those are the only dependencies.

The benchmarks provide useful google benchmark switches. It might be a good idea to run them like:
`benchmarks --benchmark_time_unit=ms --benchmark_min_time=5s`

The example usage can be found in `example.cpp`

Some remarks
========================================================================

`naive.hpp` is an initial implementation conceived to establish API and as a basis for tests to ensure correctness. It needs to lock whole queue for synchronization.

`circular_buffer.hpp` provides implementation based on a contiguous circular buffer. However, due to simplicity of implementation it's an unbounded array at the backend. This comes with some disadvantages:
1. Allocation will default initialize cells, so the user pays for that.
2. Elements cannot be constructed in-place, because the storage is already taken. They are, instead, moved in and out of the queue by pop and push.

To sum up, **limitations** are:
1. Stored `DataT` must be default constructible, and `nothrow_move_assignable`.
2. Also, currently the number of total pushes cannot exceed `std::numeric_limits<std::size_t>::max()`. 

Further obvious improvements would be:

1. Use `byte` array for storage and placement `new` or `construct_at`

--------------------------------------

Copyright - Łukasz Kucharski 2026
