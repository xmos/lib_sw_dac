cmake_minimum_required(VERSION 3.21)
include($ENV{XMOS_CMAKE_PATH}/xcommon.cmake)

project(test_sigma_delta)

set(XMOS_SANDBOX_DIR 		${CMAKE_CURRENT_LIST_DIR}/../../..)

set(APP_HW_TARGET 			XK-EVK-XU316)

set(APP_DEPENDENT_MODULES 	"lib_sw_dac"
							)

set(APP_PCA_ENABLE 			OFF) # TODO why does PCA cause build error

set(COMPILER_FLAGS_COMMON   -O3
                            -mcmodel=large
                            -g
                            -report
                            -fcmdline-buffer-bytes=1024)

set(APP_INCLUDES    		src)

set(APP_COMPILER_FLAGS_CHAN ${COMPILER_FLAGS_COMMON} -DSW_DAC_SD_TEST_MODE=1)
set(APP_COMPILER_FLAGS_PORT ${COMPILER_FLAGS_COMMON} -DSW_DAC_SD_TEST_MODE=0)


XMOS_REGISTER_APP()
