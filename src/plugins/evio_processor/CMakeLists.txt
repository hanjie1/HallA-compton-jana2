#
# CMake configuration for evio_processor plugin
#

add_jana_plugin(evio_processor
    SOURCES
        InitPlugin.cc
        JEventProcessor_EVIO.cc
        JBlockProcessor_EVIO.cc
)

target_link_libraries(evio_processor
    PRIVATE
        ${JANA_LIBRARY}
        ROOT::Core
        ROOT::RIO
        ROOT::Tree
        ROOT::Hist
        evio_parser_data_types  # data-object headers (FADC, TIScaler, MPD, VFTDC, ...)
)
