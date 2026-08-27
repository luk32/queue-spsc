Single producer single consumer queue implementation.
========================================================================

This is my toy implementation of a SPSC Queue as a template library. It supports concurrent enquing and dequing elements by two threads.

To compile, test and benchmark it requires CMake 3.15+ and clang or gcc with c++20 support.

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

This implementation doesn't lock  whole queue for reading, however for writing there still is a lock on the storage because of possible overlap in written/accessed element in case the queue is (near) empty.

Further obvious improvements would be:

1. Use `byte` array for storage and placement `new` .
2. Figure out syncronization pattern to lock storage only when queue has 0/1 elements. 

--------------------------------------

Copyright - Łukasz Kucharski 2026