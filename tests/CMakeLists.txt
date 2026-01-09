cmake_minimum_required(VERSION 3.21)
include($ENV{XMOS_CMAKE_PATH}/xcommon.cmake)
project(lib_sw_dac_tests)

add_subdirectory(timing_test_sf)
add_subdirectory(test_filter_thdn)
add_subdirectory(test_pre_distort)
add_subdirectory(test_sigma_delta)
add_subdirectory(test_exit)

