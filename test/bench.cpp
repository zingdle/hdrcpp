#include <benchmark/benchmark.h>

#include <random>

#include <hdrcpp/hdr_histgram.hpp>
#include <hdr/hdr_histogram.h>

static void BM_hdr_cpp_record_values(benchmark::State &state) {
  std::random_device rd;
  std::mt19937 gen(rd());
  // gama distribution shape 1 scale 100000
  std::gamma_distribution<double> latency_gamma_dist(1, 100000);

  hdrcpp::HdrHistogram<1, 360000, 2> h;
  int64_t max_value = 360000;
  for (auto _ : state) {
    for (int i = 0; i < 10000; i++) {
      int64_t number = int64_t(latency_gamma_dist(gen)) + 1;
      number = number > max_value ? max_value : number;
      h.record_values(number);
    }
  }
}

static void BM_hdr_c_record_values(benchmark::State &state) {
  std::random_device rd;
  std::mt19937 gen(rd());
  // gama distribution shape 1 scale 100000
  std::gamma_distribution<double> latency_gamma_dist(1, 100000);

  struct hdr_histogram *h;
  hdr_init(1, 360000, 2, &h);
  int64_t max_value = 360000;
  for (auto _ : state) {
    for (int i = 0; i < 10000; i++) {
      int64_t number = int64_t(latency_gamma_dist(gen)) + 1;
      number = number > max_value ? max_value : number;
      hdr_record_value(h, number);
    }
  }
}

// Register the functions as a benchmark
BENCHMARK(BM_hdr_cpp_record_values);
BENCHMARK(BM_hdr_c_record_values);
BENCHMARK_MAIN();