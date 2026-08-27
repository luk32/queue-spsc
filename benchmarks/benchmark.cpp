#include <algorithm>
#include <random>
#include <string>
#include <thread>

#include <benchmark/benchmark.h>

#include "../circular_buffer.hpp"
#include "../naive.hpp"

namespace {
static std::random_device rnd_dev;
static std::default_random_engine rnd_eng{rnd_dev()};

void reset_with_random_vals(std::vector<int> &vect) {
  std::ranges::generate(vect, [] { return rnd_eng(); });
}

void reset_with_random_vals(std::vector<std::string> &vect) {
  static const char alphabet[] = "qwertyuiopasdfghjklzxcvbnm";
  static std::uniform_int_distribution<> alpha_distrib(0, sizeof(alphabet) - 1);
  for (auto &s : vect) {
    const auto str_size = rnd_eng() % 1024;
    s.reserve(str_size);
    for (auto i = 0; i < str_size; ++i)
      s[i] = alphabet[alpha_distrib(rnd_eng)];
  }
}
} // namespace

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
  state.SetItemsProcessed(state.range() * state.iterations());
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
  state.SetItemsProcessed(state.range() * state.iterations());
}

template <typename Q> static void BM_SequenialPushPop(benchmark::State &state) {
  using Value = typename Q::ValueT;

  std::vector<Value> random_ints(state.range());
  std::size_t pop_count;
  Q q(state.range());

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

    typename Q::ValueT e;
    while (pop_count++ < state.range()) {
      q.tryPop(e);
    }
  }

  if constexpr (std::is_integral_v<Value>)
    state.SetBytesProcessed(state.range() * state.iterations() * sizeof(Value));
  state.SetItemsProcessed(state.range() * state.iterations());
}

template <typename Q> static void BM_2ThreadPushPop(benchmark::State &state) {
  using Value = typename Q::ValueT;

  std::vector<Value> random_ints(state.range());
  std::size_t pop_count;
  Q q(state.range());

  for (auto _ : state) {
    state.PauseTiming();
    reset_with_random_vals(random_ints);
    for (const auto &i : random_ints) {
      q.tryPush(i);
    }
    state.ResumeTiming();

    std::jthread producer([&] {
      for (const auto &e : random_ints) {
        q.tryPush(e);
      }
    });

    std::jthread consumer([&] {
      Value e;
      while (pop_count++ < state.range()) {
        q.tryPop(e);
      }
    });
  }

  if constexpr (std::is_integral_v<Value>)
    state.SetBytesProcessed(state.range() * state.iterations() * sizeof(Value));
  state.SetItemsProcessed(state.range() * state.iterations());
}

/* --------- int benchmarks ---------------------------*/
BENCHMARK(BM_PushOnly<NaiveQueue>)->RangeMultiplier(16)->Range(16, 1 << 28);
BENCHMARK(BM_PopOnly<NaiveQueue>)->RangeMultiplier(16)->Range(16, 1 << 28);
BENCHMARK(BM_SequenialPushPop<NaiveQueue<int>>)
    ->RangeMultiplier(16)
    ->Range(16, 1 << 28);
BENCHMARK(BM_2ThreadPushPop<NaiveQueue<int>>)
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
BENCHMARK(BM_SequenialPushPop<CirularBufferQueue<int>>)
    ->RangeMultiplier(16)
    ->Range(16, 1 << 28);
BENCHMARK(BM_2ThreadPushPop<CirularBufferQueue<int>>)
    ->ThreadRange(1, 2)
    ->RangeMultiplier(16)
    ->Range(16, 1 << 28)
    ->UseRealTime();

/* --------- std::string benchmarks ---------------------------*/
BENCHMARK(BM_2ThreadPushPop<NaiveQueue<std::string>>)
    ->ThreadRange(1, 2)
    ->RangeMultiplier(16)
    ->Range(16, 1 << 20)
    ->UseRealTime();
BENCHMARK(BM_2ThreadPushPop<CirularBufferQueue<std::string>>)
    ->ThreadRange(1, 2)
    ->RangeMultiplier(16)
    ->Range(16, 1 << 20)
    ->UseRealTime();
BENCHMARK_MAIN();
