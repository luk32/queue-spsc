add_executable(benchmarks benchmark.cpp)

target_link_libraries(benchmarks PRIVATE
  benchmark::benchmark
  benchmark::benchmark_main)