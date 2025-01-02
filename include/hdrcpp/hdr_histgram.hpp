// Modified from: https://github.com/HdrHistogram/HdrHistogram_c and https://github.com/HdrHistogram/HdrHistogram_rust

#include <array>

#include "gcem/gcem.hpp"

#ifdef __GNUC__
#define hdr_unlikely(x) (__builtin_expect(!!(x), 0))
#else
#define hdr_unlikely(x) (x)
#endif

namespace hdrcpp {

inline int32_t count_leading_zeros_64(int64_t value) {
#if defined(_MSC_VER) && !(defined(__clang__) && (defined(_M_ARM) || defined(_M_ARM64)))
  uint32_t leading_zero = 0;
#if defined(_WIN64)
  _BitScanReverse64(&leading_zero, value);
#else
  uint32_t high = value >> 32;
  if (_BitScanReverse(&leading_zero, high)) {
    leading_zero += 32;
  } else {
    uint32_t low = value & 0x00000000FFFFFFFF;
    _BitScanReverse(&leading_zero, low);
  }
#endif
  return 63 - leading_zero; /* smallest power of 2 containing value */
#else
  return __builtin_clzll(value); /* smallest power of 2 containing value */
#endif
}

template <int64_t lowest_discernible_value, int64_t highest_trackable_value, int significant_figures>
struct HdrHistogram {
  /// `Histogram` is the core data structure in HdrSample. It records values, and performs analytics.
  ///
  /// At its heart, it keeps the count for recorded samples in "buckets" of values. The resolution
  /// and distribution of these buckets is tuned based on the desired highest trackable value, as
  /// well as the user-specified number of significant decimal digits to preserve. The values for the
  /// buckets are kept in a way that resembles floats and doubles: there is a mantissa and an
  /// exponent, and each bucket represents a different exponent. The "sub-buckets" within a bucket
  /// represent different values for the mantissa.
  ///
  /// To a first approximation, the sub-buckets of the first
  /// bucket would hold the values `0`, `1`, `2`, `3`, …, the sub-buckets of the second bucket would
  /// hold `0`, `2`, `4`, `6`, …, the third would hold `0`, `4`, `8`, and so on. However, the low
  /// half of each bucket (except bucket 0) is unnecessary, since those values are already covered by
  /// the sub-buckets of all the preceeding buckets. Thus, `Histogram` keeps the top half of every
  /// such bucket.
  ///
  /// For the purposes of explanation, consider a `Histogram` with 2048 sub-buckets for every bucket,
  /// and a lowest discernible value of 1:
  ///
  /// <pre>
  /// The 0th bucket covers 0...2047 in multiples of 1, using all 2048 sub-buckets
  /// The 1st bucket covers 2048..4095 in multiples of 2, using only the top 1024 sub-buckets
  /// The 2nd bucket covers 4096..8191 in multiple of 4, using only the top 1024 sub-buckets
  /// The 3rd bucket covers 8192..16383 in multiple of 8, using only the top 1024 sub-buckets
  /// ...
  /// </pre>
  ///
  /// Bucket 0 is "special" here. It is the only one that has 2048 entries. All the rest have
  /// 1024 entries (because their bottom half overlaps with and is already covered by the all of
  /// the previous buckets put together). In other words, the `k`'th bucket could represent `0 *
  /// 2^k` to `2048 * 2^k` in 2048 buckets with `2^k` precision, but the midpoint of `1024 * 2^k
  /// = 2048 * 2^(k-1)`, which is the k-1'th bucket's end. So, we would use the previous bucket
  /// for those lower values as it has better precision.
  ///

  static_assert(lowest_discernible_value >= 1);
  static_assert(lowest_discernible_value * 2 <= highest_trackable_value);
  static_assert(significant_figures >= 1 && significant_figures <= 5);

  static constexpr int64_t largest_value_with_single_unit_resolution = 2 * gcem::pow(10, significant_figures);
  static constexpr int32_t sub_bucket_count_magnitude = static_cast<int32_t>(
      gcem::ceil((gcem::log(static_cast<double>(largest_value_with_single_unit_resolution)) / gcem::log(2))));
  static constexpr int32_t sub_bucket_half_count_magnitude = std::max(sub_bucket_count_magnitude - 1, 0);

  static constexpr double unit_magnitude_ = gcem::log(static_cast<double>(lowest_discernible_value)) / gcem::log(2);
  static_assert(unit_magnitude_ <= INT32_MAX);
  static constexpr int32_t unit_magnitude = static_cast<int32_t>(unit_magnitude_);

  static constexpr int32_t sub_bucket_count = static_cast<int32_t>(gcem::pow(2, sub_bucket_half_count_magnitude + 1));
  static constexpr int32_t sub_bucket_half_count = sub_bucket_count / 2;
  static constexpr int64_t sub_bucket_mask = (static_cast<int64_t>(sub_bucket_count) - 1) << unit_magnitude;

  static_assert(unit_magnitude + sub_bucket_half_count_magnitude <= 61);

  static constexpr auto buckets_needed_to_cover_value = []() -> int32_t {
    int64_t smallest_untrackable_value = (static_cast<int64_t>(sub_bucket_count)) << unit_magnitude;
    int32_t buckets_needed = 1;
    while (smallest_untrackable_value <= highest_trackable_value) {
      if (smallest_untrackable_value > INT64_MAX / 2) {
        return buckets_needed + 1;
      }
      smallest_untrackable_value <<= 1;
      buckets_needed++;
    }

    return buckets_needed;
  };

  static constexpr int32_t bucket_count = buckets_needed_to_cover_value();
  static constexpr int32_t counts_len = (bucket_count + 1) * (sub_bucket_count / 2);

  static int32_t get_bucket_index(int64_t value) {
    int32_t pow2ceiling = 64 - count_leading_zeros_64(value | sub_bucket_mask);
    return pow2ceiling - unit_magnitude - (sub_bucket_half_count_magnitude + 1);
  }

  static int32_t get_sub_bucket_index(int64_t value, int32_t bucket_index, int32_t unit_magnitude) {
    return static_cast<int32_t>(value >> (bucket_index + unit_magnitude));
  }

  static int32_t counts_index(int32_t bucket_index, int32_t sub_bucket_index) {
    int32_t bucket_base_index = (bucket_index + 1) << sub_bucket_half_count_magnitude;
    int32_t offset_in_bucket = sub_bucket_index - sub_bucket_half_count;
    return bucket_base_index + offset_in_bucket;
  }

  static int32_t counts_index_for(int64_t value) {
    int32_t bucket_index = get_bucket_index(value);
    int32_t sub_bucket_index = get_sub_bucket_index(value, bucket_index, unit_magnitude);

    return counts_index(bucket_index, sub_bucket_index);
  }

  static int64_t size_of_equivalent_value_range(int64_t value) {
    int32_t bucket_index = get_bucket_index(value);
    int32_t sub_bucket_index = get_sub_bucket_index(value, bucket_index, unit_magnitude);
    int32_t adjusted_bucket = (sub_bucket_index >= sub_bucket_count) ? (bucket_index + 1) : bucket_index;
    return 1ll << (unit_magnitude + adjusted_bucket);
  }

  static int64_t next_non_equivalent_value(int64_t value) {
    return lowest_equivalent_value(value) + size_of_equivalent_value_range(value);
  }

  static int64_t highest_equivalent_value(int64_t value) { return next_non_equivalent_value(value) - 1; }

  static int64_t lowest_equivalent_value(int64_t value) {
    int32_t bucket_index = get_bucket_index(value);
    int32_t sub_bucket_index = get_sub_bucket_index(value, bucket_index, unit_magnitude);
    return value_from_index(bucket_index, sub_bucket_index, unit_magnitude);
  }

  static int64_t value_at_index(int32_t index) {
    int32_t bucket_index = (index >> sub_bucket_half_count_magnitude) - 1;
    int32_t sub_bucket_index = (index & (sub_bucket_half_count - 1)) + sub_bucket_half_count;

    if (bucket_index < 0) {
      sub_bucket_index -= sub_bucket_half_count;
      bucket_index = 0;
    }

    return value_from_index(bucket_index, sub_bucket_index, unit_magnitude);
  }

  static int64_t value_from_index(int32_t bucket_index, int32_t sub_bucket_index, int32_t unit_magnitude) {
    return (static_cast<int64_t>(sub_bucket_index)) << (bucket_index + unit_magnitude);
  }

  std::array<int64_t, counts_len> counts{0};
  int64_t total_count = 0;
  int64_t min_value = INT64_MAX;
  int64_t max_value = 0;

  bool record_values(int64_t value, int64_t count = 1) {
    if (hdr_unlikely(value < 0 || highest_trackable_value < value)) {
      return false;
    }

    int32_t counts_index = counts_index_for(value);
    if (hdr_unlikely(counts_index < 0 || counts_len <= counts_index)) {
      return false;
    }

    counts[counts_index] += count;
    total_count += count;

    min_value = (value < min_value && value != 0) ? value : min_value;
    max_value = (value > max_value) ? value : max_value;

    return true;
  }

  int64_t value_at_percentile(double percentile) {
    auto get_value_from_idx_up_to_count = [this](int64_t count_at_percentile) -> int64_t {
      int64_t count_to_idx = 0;
      count_at_percentile = 0 < count_at_percentile ? count_at_percentile : 1;
      for (int32_t idx = 0; idx < counts_len; idx++) {
        count_to_idx += counts[idx];
        if (count_to_idx >= count_at_percentile) return value_at_index(idx);
      }
      return 0;
    };

    double requested_percentile = percentile < 100.0 ? percentile : 100.0;
    int64_t count_at_percentile = static_cast<int64_t>(((requested_percentile / 100) * total_count) + 0.5);
    int64_t value_from_idx = get_value_from_idx_up_to_count(count_at_percentile);
    if (percentile == 0.0) {
      return lowest_equivalent_value(value_from_idx);
    }
    return highest_equivalent_value(value_from_idx);
  }

  template <typename... Ts>
  auto value_at_percentiles(Ts... percentiles) {
    constexpr size_t N = sizeof...(Ts);
    auto construct_impl = [total_count = total_count](double percentile) {
      const double requested_percentile = percentile < 100.0 ? percentile : 100.0;
      const int64_t count_at_percentile = static_cast<int64_t>(((requested_percentile / 100) * total_count) + 0.5);
      return count_at_percentile > 1 ? count_at_percentile : 1;
    };
    std::array<int64_t, N> values{construct_impl(percentiles)...};

    size_t at_pos = 0;
    int64_t total = 0;
    int32_t counts_index = 0;
    while (counts_index < counts_len && at_pos < N) {
      total += counts[counts_index];
      int64_t value = value_at_index(counts_index);
      counts_index++;

      while (at_pos < N && total >= values[at_pos]) {
        values[at_pos] = highest_equivalent_value(value);
        at_pos++;
      }
    }
    return values;
  }
};

}  // namespace hdrcpp
