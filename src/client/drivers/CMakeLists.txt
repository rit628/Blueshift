# must be an object library to ensure strong symbols for controller dependent drivers are utilized
bls_add_library(drivers OBJECT LINKS devtypes dynamic_message network)