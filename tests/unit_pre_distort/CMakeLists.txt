cmake_minimum_required(VERSION 3.21)
include($ENV{XMOS_CMAKE_PATH}/xcommon.cmake)

project(test_unit_pre_distort)

set(XMOS_SANDBOX_DIR 		${CMAKE_CURRENT_LIST_DIR}/../../..)

set(APP_HW_TARGET 			XCORE-AI-EXPLORER)

set(APP_DEPENDENT_MODULES 	"lib_sw_dac")

set(APP_PCA_ENABLE 			ON)

set(APP_COMPILER_FLAGS    	-O3
							-g
							-report)

set(APP_INCLUDES    		src)


XMOS_REGISTER_APP()
