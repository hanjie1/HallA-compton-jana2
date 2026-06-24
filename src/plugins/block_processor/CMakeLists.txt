#
# CMake configuration for evio_processor plugin
#

add_jana_plugin(block_processor
    SOURCES
        InitPlugin.cc
        JBlockProcessor_EVIO.cc
)


target_link_libraries(block_processor
    PRIVATE
        ${JANA_LIBRARY}
        ROOT::Core
        ROOT::RIO
        ROOT::Tree
        ROOT::Hist
        evio_parser_data_types  # data-object headers (FADC, TIScaler, MPD, VFTDC, ...)
	evio::eviocc
)
