cmake_minimum_required(VERSION 3.21)
include($ENV{XMOS_CMAKE_PATH}/xcommon.cmake)

project(timing_test_sf)

set(XMOS_SANDBOX_DIR 		${CMAKE_CURRENT_LIST_DIR}/../../..)

set(APP_HW_TARGET 			XK-EVK-XU316)

set(APP_DEPENDENT_MODULES 	"lib_sw_dac"
							)

set(APP_PCA_ENABLE 			OFF) # TODO why does PCA cause build error

set(APP_COMPILER_FLAGS    	-O3
                                -mcmodel=large
				-g
				-report
				-fcmdline-buffer-bytes=1024)

set(APP_INCLUDES    		src)


XMOS_REGISTER_APP()
