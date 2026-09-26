# Header-only interfaces plugins publish through ServiceExchange; each vtable lives in the implementing plugin.
add_library(cs2-contracts INTERFACE)
target_include_directories(cs2-contracts INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")
