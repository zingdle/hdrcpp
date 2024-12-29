set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "Disable Google Benchmark tests")
add_subdirectory(3rd/benchmark)
add_subdirectory(3rd/HdrHistogram_c)
add_subdirectory(3rd/fmt)

add_executable(bench bench.cpp)
target_link_libraries(bench PRIVATE benchmark::benchmark hdr_histogram hdrcpp_histogram)

add_executable(demo demo.cpp)
target_link_libraries(demo PRIVATE hdrcpp_histogram fmt)