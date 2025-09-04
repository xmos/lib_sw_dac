set(LIB_NAME lib_template)
set(LIB_VERSION 1.0.0)
set(LIB_INCLUDES api)

set(LIB_DEPENDENT_MODULES "")

set(LIB_OPTIONAL_HEADERS template_conf.h)

set(LIB_COMPILER_FLAGS -O3
                       -g)

XMOS_REGISTER_MODULE()
