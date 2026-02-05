# link against all driver objects to ensure correct implementations are used for target platform
bls_add_library(device STATIC LINKS $<TARGET_OBJECTS:drivers> network dynamic_message TSM)
bls_add_library(devtypes INTERFACE INCLUDES devtypes)