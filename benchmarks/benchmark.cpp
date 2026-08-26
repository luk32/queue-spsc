#include <algorithm>
#include <random>

#include <benchmark/benchmark.h>
#include <thread>

#include "../circular_buffer.hpp"
#include "../naive.hpp"

void reset_with_random_vals(std::vector<int> &vect) {
  static std::random_device rnd_dev;
  static std::default_random_engine rnd_eng{rnd_dev()};
  std::ranges::generate(vect, [] { return rnd_eng(); });
}

template <template <typename> typename Q>
static void BM_PushOnly(benchmark::State &state) {
  Q<int> q(state.range());
  std::vector<int> random_ints(state.range());

  for (auto _ : state) {
    state.PauseTiming();
    reset_with_random_vals(random_ints);
    state.ResumeTiming();
    for (const auto &i : random_ints) {
      q.tryPush(i);
    }
  }
}

template <template <typename> typename Q>
static void BM_PopOnly(benchmark::State &state) {
  Q<int> q(state.range());
  std::vector<int> random_ints(state.range());

  for (auto _ : state) {
    state.PauseTiming();
    reset_with_random_vals(random_ints);
    for (const auto &i : random_ints) {
      q.tryPush(i);
    }
    state.ResumeTiming();
    for (const auto &i : random_ints) {
      int e;
      q.tryPop(e);
    }
  }
}

template <template <typename> typename Q>
static void BM_SequenialPushPop(benchmark::State &state) {
  Q<int> q(state.range());
  std::vector<int> random_ints(state.range());
  std::size_t pop_count;

  for (auto _ : state) {
    state.PauseTiming();
    reset_with_random_vals(random_ints);
    for (const auto &i : random_ints) {
      q.tryPush(i);
    }
    state.ResumeTiming();

    for (const auto &e : random_ints) {
      q.tryPush(e);
    }

    int e;
    while (pop_count++ < state.range()) {
      q.tryPop(e);
    }
  }

  state.SetBytesProcessed(state.range() * state.iterations() * sizeof(int));
}

template <template <typename> typename Q>
static void BM_2ThreadPushPop(benchmark::State &state) {
  Q<int> q(state.range());
  std::vector<int> random_ints(state.range());
  std::size_t pop_count;

  for (auto _ : state) {
    state.PauseTiming();
    reset_with_random_vals(random_ints);
    for (const auto &i : random_ints) {
      q.tryPush(i);
    }
    state.ResumeTiming();

    std::thread producer([&] {
      for (const auto &e : random_ints) {
        q.tryPush(e);
      }
    });

    std::thread consumer([&] {
      int e;
      while (pop_count++ < state.range()) {
        q.tryPop(e);
      }
    });

    producer.join();
    consumer.join();
  }
  state.SetBytesProcessed(state.range() * state.iterations() * sizeof(int));
}

BENCHMARK(BM_PushOnly<NaiveQueue>)->RangeMultiplier(16)->Range(16, 1 << 28);
BENCHMARK(BM_PopOnly<NaiveQueue>)->RangeMultiplier(16)->Range(16, 1 << 28);
BENCHMARK(BM_SequenialPushPop<NaiveQueue>)
    ->RangeMultiplier(16)
    ->Range(16, 1 << 28);
BENCHMARK(BM_2ThreadPushPop<NaiveQueue>)
    ->ThreadRange(1, 2)
    ->RangeMultiplier(16)
    ->Range(16, 1 << 28)
    ->UseRealTime();

BENCHMARK(BM_PushOnly<CirularBufferQueue>)
    ->RangeMultiplier(16)
    ->Range(16, 1 << 28);
BENCHMARK(BM_PopOnly<CirularBufferQueue>)
    ->RangeMultiplier(16)
    ->Range(16, 1 << 28);
BENCHMARK(BM_SequenialPushPop<CirularBufferQueue>)
    ->RangeMultiplier(16)
    ->Range(16, 1 << 28);
BENCHMARK(BM_2ThreadPushPop<CirularBufferQueue>)
    ->ThreadRange(1, 2)
    ->RangeMultiplier(16)
    ->Range(16, 1 << 28)
    ->UseRealTime();

BENCHMARK_MAIN();