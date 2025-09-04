set(LIB_NAME 					lib_sw_dac)

set(LIB_VERSION 				0.1.0)

set(LIB_INCLUDES 				api
								src
								src/filters
								src/standard_fidelity)

set(LIB_DEPENDENT_MODULES 		"")

set(LIB_COMPILER_FLAGS 			-O3
								-g
								-Wall
								-Wextra)

set(LIB_OPTIONAL_HEADERS 		sw_dac_conf.h)

XMOS_REGISTER_MODULE()
