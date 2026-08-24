# Interfaces plugins publish to each other through VoltMod's ServiceExchange. Header-only:
# the vtable lives in whichever plugin implements it.
add_library(cs2-contracts INTERFACE)
target_include_directories(cs2-contracts INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_compile_features(cs2-contracts INTERFACE cxx_std_23)
