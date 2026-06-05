#
# CMake configuration for MOLLERADC module parser
#

add_library(molleradc_parser STATIC
    ModuleParser_MOLLERADC.cc
)

target_include_directories(molleradc_parser
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/data_objects
)

target_link_libraries(molleradc_parser
    PUBLIC
        core
)

set(MOLLERADC_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/data_objects PARENT_SCOPE)

file(GLOB MOLLERADC_PUBLIC_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/data_objects/*.h
)

set(MOLLERADC_PUBLIC_HEADERS ${MOLLERADC_PUBLIC_HEADERS} PARENT_SCOPE)
