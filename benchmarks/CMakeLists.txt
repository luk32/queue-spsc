add_executable(int_bench benchmark.cpp)

target_link_libraries(int_bench PRIVATE
  benchmark::benchmark
  benchmark::benchmark_main)