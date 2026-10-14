# dcdata extracts the game's data from a disc image into a plain directory.
# It shares the port's language settings but none of the game's headers.
file(GLOB DCDATA_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
add_executable(dcdata ${DCDATA_SOURCES})
set_target_properties(dcdata PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
target_compile_options(dcdata PRIVATE -Wall -Wextra)
target_link_options(dcdata PRIVATE -fuse-ld=lld)
# Chronicle-Expanded: the files it derives from the extracted data, built at extraction.
if(CHRONICLE_EXPANDED)
    file(GLOB EXPANDED_DATA_SOURCES CONFIGURE_DEPENDS ${EXPANDED_DIR}/src/data/*.cpp)
    target_sources(dcdata PRIVATE ${EXPANDED_DATA_SOURCES})
    target_compile_definitions(dcdata PRIVATE CHRONICLE_EXPANDED)
    target_include_directories(dcdata PRIVATE ${EXPANDED_DIR}/include)
    set_source_files_properties(${EXPANDED_DATA_SOURCES} PROPERTIES COMPILE_OPTIONS -ffp-contract=off)
endif()

include(GNUInstallDirs)
install(TARGETS dcdata RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
