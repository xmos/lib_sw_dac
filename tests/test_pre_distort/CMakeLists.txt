cmake_minimum_required(VERSION 3.21)
include($ENV{XMOS_CMAKE_PATH}/xcommon.cmake)

project(test_pre_distort)

set(XMOS_SANDBOX_DIR 		${CMAKE_CURRENT_LIST_DIR}/../../..)

set(APP_HW_TARGET 			XK-EVK-XU316)

set(APP_DEPENDENT_MODULES 	"lib_sw_dac")

set(APP_PCA_ENABLE 			OFF)

set(APP_COMPILER_FLAGS          -O3
                                -mcmodel=large
				-g
				-report)

set(APP_INCLUDES    		src)


XMOS_REGISTER_APP()
