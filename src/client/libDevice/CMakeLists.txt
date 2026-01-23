bls_add_library(device STATIC $<TARGET_OBJECTS:drivers> LINKS network dynamic_message TSM)
bls_add_library(devtypes INTERFACE INCLUDES devtypes)