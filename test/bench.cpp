#include <benchmark/benchmark.h>
#include <hdr/hdr_histogram.h>

#include <hdrcpp/hdr_histgram.hpp>
#include <random>

static std::vector<int64_t> generate_random_latency(int n, int64_t max_value) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::gamma_distribution<double> latency_gamma_dist(1, 100000);

  std::vector<int64_t> latency(n);
  for (int i = 0; i < n; i++) {
    int64_t number = int64_t(latency_gamma_dist(gen)) + 1;
    number = number > max_value ? max_value : number;
    latency[i] = number;
  }

  return latency;
}

static std::vector<int64_t> generate_random_percentile(int n) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> dist(0, 100);

  std::vector<int64_t> percentile(n);
  for (int i = 0; i < n; i++) {
    percentile[i] = dist(gen);
  }

  return percentile;
}

static void BM_hdr_cpp_init(benchmark::State &state) {
  for (auto _ : state) {
    benchmark::DoNotOptimize(hdrcpp::HdrHistogram<1, 360000, 2>());
    benchmark::ClobberMemory();
  }
}

static void BM_hdr_cpp_new_init(benchmark::State &state) {
  for (auto _ : state) {
    benchmark::DoNotOptimize(new hdrcpp::HdrHistogram<1, 360000, 2>());
    benchmark::ClobberMemory();
  }
}

static void BM_hdr_c_init(benchmark::State &state) {
  for (auto _ : state) {
    struct hdr_histogram *histogram;
    benchmark::DoNotOptimize(hdr_init(1, 360000, 2, &histogram));
    benchmark::ClobberMemory();
  }
}

static void BM_hdr_cpp_record_values(benchmark::State &state) {
  const int N = state.range(0);
  auto latency = generate_random_latency(N, 360000);

  hdrcpp::HdrHistogram<1, 360000, 2> h;
  for (auto _ : state) {
    for (int i = 0; i < N; i++) {
      benchmark::DoNotOptimize(h.record_values(latency[i]));
      benchmark::ClobberMemory();
    }
  }
}

static void BM_hdr_c_record_values(benchmark::State &state) {
  const int N = state.range(0);
  auto latency = generate_random_latency(N, 360000);

  struct hdr_histogram *h;
  hdr_init(1, 360000, 2, &h);
  for (auto _ : state) {
    for (int i = 0; i < N; i++) {
      benchmark::DoNotOptimize(hdr_record_value(h, latency[i]));
      benchmark::ClobberMemory();
    }
  }
}

static void BM_hdr_cpp_value_at_percentile(benchmark::State &state) {
  const int N = state.range(0);
  auto latency = generate_random_latency(N, 360000);
  auto percentile = generate_random_percentile(N);

  hdrcpp::HdrHistogram<1, 360000, 2> h;

  for (int i = 0; i < N; i++) {
    h.record_values(latency[i]);
  }

  for (auto _ : state) {
    for (int i = 0; i < N; i++) {
      benchmark::DoNotOptimize(h.value_at_percentile(percentile[i]));
      benchmark::ClobberMemory();
    }
  }
}

static void BM_hdr_c_value_at_percentile(benchmark::State &state) {
  const int N = state.range(0);
  auto latency = generate_random_latency(N, 360000);
  auto percentile = generate_random_percentile(N);

  struct hdr_histogram *h;
  hdr_init(1, 360000, 2, &h);

  for (int i = 0; i < N; i++) {
    hdr_record_value(h, latency[i]);
  }

  for (auto _ : state) {
    for (int i = 0; i < N; i++) {
      benchmark::DoNotOptimize(hdr_value_at_percentile(h, percentile[i]));
      benchmark::ClobberMemory();
    }
  }
}

// Register the functions as a benchmark
BENCHMARK(BM_hdr_cpp_init);
BENCHMARK(BM_hdr_cpp_new_init);
BENCHMARK(BM_hdr_c_init);
BENCHMARK(BM_hdr_cpp_record_values)->RangeMultiplier(10)->Range(1000, 100000);
BENCHMARK(BM_hdr_c_record_values)->RangeMultiplier(10)->Range(1000, 100000);
BENCHMARK(BM_hdr_cpp_value_at_percentile)->RangeMultiplier(10)->Range(1000, 100000);
BENCHMARK(BM_hdr_c_value_at_percentile)->RangeMultiplier(10)->Range(1000, 100000);
BENCHMARK_MAIN();