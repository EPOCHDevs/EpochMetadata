include_guard(GLOBAL)

add_library(tulip_indicators STATIC IMPORTED GLOBAL)
set_target_properties(tulip_indicators PROPERTIES
        IMPORTED_LOCATION "${DEPENDENCY_DIR}/tulipindicators/libindicators.a" # Adjusted path for the library
        INTERFACE_INCLUDE_DIRECTORIES "${DEPENDENCY_DIR}/tulipindicators")