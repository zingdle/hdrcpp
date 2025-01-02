#include <fmt/format.h>

#include <hdrcpp/hdr_histgram.hpp>

using HdrHistT = hdrcpp::HdrHistogram<1, 8191, 3>;

int main() {
  HdrHistT h;
  h.record_values(2);
  h.record_values(2046);
  h.record_values(2047);
  h.record_values(2048);
  h.record_values(2049);
  h.record_values(4094);
  h.record_values(4095);
  h.record_values(4096);
  h.record_values(4097);

  fmt::print("sizeof(h): {}\n", sizeof(h));
  fmt::print("unit_magnitude:{}\n", HdrHistT::unit_magnitude);
  fmt::print("largest_value_with_single_unit_resolution:{}\n", HdrHistT::largest_value_with_single_unit_resolution);
  fmt::print("sub_bucket_count:{}\n", HdrHistT::sub_bucket_count);
  fmt::print("sub_bucket_count_magnitude:{}\n", HdrHistT::sub_bucket_count_magnitude);
  fmt::print("sub_bucket_half_count:{}\n", HdrHistT::sub_bucket_half_count);
  fmt::print("sub_bucket_half_count_magnitude:{}\n", HdrHistT::sub_bucket_half_count_magnitude);
  fmt::print("sub_bucket_mask:{}\n", HdrHistT::sub_bucket_mask);
  fmt::print("bucket_count:{}\n", HdrHistT::bucket_count);
  fmt::print("counts_len:{}\n", HdrHistT::counts_len);

  fmt::print("P50: {}\n", h.value_at_percentile(50));
  fmt::print("P75: {}\n", h.value_at_percentile(75));
  fmt::print("P90: {}\n", h.value_at_percentile(90));
  fmt::print("P99: {}\n", h.value_at_percentile(99));

  auto percentiles = h.value_at_percentiles(50, 75, 90, 99);
  fmt::print("P50/P75/P90/P99: {} {} {} {}\n", percentiles[0], percentiles[1], percentiles[2], percentiles[3]);
}