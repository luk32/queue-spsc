Single producer single consumer queue implementation.
========================================================================

This is my toy implementation of a SPSC Queue as a template library.

To compile, test and benchmark it requires CMake 3.15+ and clang or gcc with c++20 support.

You can do it by executing:
```
cmake -Bbuild -DCMAKE_BUILD_TYPE=Release
cmake --build build/
cd build
ctest
./benchmarks/int_bench
```

It will pull main branch of google test and google benchmark. Those are the only dependencies.

Copyright - Łukasz Kucharski 2026