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

template <typename IntT>
void reset_with_random_vals(std::vector<IntT> &vect) {
  std::ranges::generate(vect, [] { return (IntT)rnd_eng(); });
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

template <std::size_t PayLoadSize = 8 * 1024>
struct Beefy {
  int i;
  std::byte data[PayLoadSize];
};

template <std::size_t PayLoadSize>
void reset_with_random_vals(std::vector<Beefy<PayLoadSize>> &vect) {
  auto i = std::size_t{0};
  for (auto &c : vect) {
    c.i = i++;
    for (auto i = 0; i < sizeof(Beefy<PayLoadSize>::data); ++i)
      c.data[i] = (std::byte)(rnd_eng() % sizeof(std::byte));
  }
}
} // namespace

static void BM_MemoryWriteThroughput(benchmark::State &state) {
  const size_t num_elements = state.range(0);
  std::vector<int64_t> data(num_elements);

  for (auto _ : state) {
    // Główna operacja zapisu pamięci
    for (size_t i = 0; i < num_elements; ++i) {
      data[i] = i;
    }
    // Zapobiega usunięciu zapisu przez optymalizacje kompilatora
    benchmark::DoNotOptimize(data.data());
    benchmark::ClobberMemory();
  }
  const int64_t bytes_processed = num_elements * sizeof(int64_t);
  state.SetBytesProcessed(bytes_processed);
}

static void BM_MemoryReadThroughput(benchmark::State &state) {
  const size_t num_elements = state.range(0);
  std::vector<int64_t> data(num_elements, 42);

  for (auto _ : state) {
    int64_t sum = 0;
    for (size_t i = 0; i < num_elements; ++i) {
      sum += data[i];
    }
    benchmark::DoNotOptimize(sum);
  }

  const int64_t bytes_processed = num_elements * sizeof(int64_t);
  state.SetBytesProcessed(bytes_processed);
}

// Some base line benchmark, this is up to 8GB. On my system 16GB had better
// performance but VM got unstable.
BENCHMARK(BM_MemoryWriteThroughput)
    ->RangeMultiplier(8)
    ->Range(1024, 512 * 1024 * 1024u);
BENCHMARK(BM_MemoryReadThroughput)
    ->RangeMultiplier(8)
    ->Range(1024, 512 * 1024 * 1024u);

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

template <typename Q>
static void BM_SequenialPushPop(benchmark::State &state) {
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

template <typename Q>
static void BM_2ThreadPushPop(benchmark::State &state) {
  using Value = typename Q::ValueT;

  std::vector<Value> random_ints(state.range());
  std::size_t pop_count;
  Q q(state.range());

  for (auto _ : state) {
    reset_with_random_vals(random_ints);

    // Using manual timing because Start/Pause timing has too much overhead
    auto start = std::chrono::high_resolution_clock::now();
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

    producer.join();
    consumer.join();

    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed_seconds =
        std::chrono::duration_cast<std::chrono::duration<double>>(end - start);

    state.SetIterationTime(elapsed_seconds.count());
  }

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

/* --------- beefy Data benchmarks ---------------------------*/
// by default Beefy uses 8kb of payload
BENCHMARK(BM_2ThreadPushPop<NaiveQueue<Beefy<128>>>)
    ->ThreadRange(1, 2)
    ->RangeMultiplier(8)
    ->Range(16, 1 << 16)
    ->UseRealTime();
BENCHMARK(BM_2ThreadPushPop<CirularBufferQueue<Beefy<128>>>)
    ->ThreadRange(1, 2)
    ->RangeMultiplier(8)
    ->Range(16, 1 << 16)
    ->UseRealTime();

BENCHMARK_MAIN();