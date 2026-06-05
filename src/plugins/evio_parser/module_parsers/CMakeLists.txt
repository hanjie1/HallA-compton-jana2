#
# Central configuration for all module parser libraries
#

# Add individual module parser subdirectories
add_subdirectory(FADC)
add_subdirectory(FADCScaler)
add_subdirectory(TIScaler)
add_subdirectory(helicity_decoder)
add_subdirectory(MPD)
add_subdirectory(VFTDC)
add_subdirectory(MOLLERADC)

# Collect all module parser libraries in a single variable that the top-level
# CMakeLists.txt can link against. When adding a new parser library, extend
# this list here instead of touching the main CMakeLists.txt
set(MODULE_PARSERS_LIBS
    fadc_parser
    fadc_scaler_parser
    tiscaler_parser
    mpd_parser
    helicity_decoder_parsers
    vftdc_parser
    molleradc_parser
    PARENT_SCOPE
)

# Collect all module parser include directories. When adding a new parser, add its
# *_INCLUDE_DIR variable here instead of touching the main CMakeLists.txt
set(MODULE_PARSERS_INCLUDE_DIRS
    ${FADC_INCLUDE_DIR}
    ${FADC_SCALER_INCLUDE_DIR}
    ${TISCALER_INCLUDE_DIR}
    ${MPD_INCLUDE_DIR}
    ${HELDEC_INCLUDE_DIR}
    ${VFTDC_INCLUDE_DIR}
    ${MOLLERADC_INCLUDE_DIR}
    PARENT_SCOPE
)

# Collect all module parser public headers. When adding a new parser, add its
# *_PUBLIC_HEADERS variable here instead of touching the main CMakeLists.txt
set(MODULE_PARSERS_HEADERS
    ${FADC_PUBLIC_HEADERS}
    ${FADC_SCALER_PUBLIC_HEADERS}
    ${TISCALER_PUBLIC_HEADERS}
    ${MPD_PUBLIC_HEADERS}
    ${HELDEC_PUBLIC_HEADERS}
    ${VFTDC_PUBLIC_HEADERS}
    ${MOLLERADC_PUBLIC_HEADERS}
    PARENT_SCOPE
)

