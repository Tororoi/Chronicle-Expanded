# The data builders' fingerprint, expanded/data/data_version.hpp's kExpandedDataVersion: a hash of their sources (line
# endings aside), which a data directory is stamped with when they build into it (expanded/data/data.hpp). CMake
# configures again when one of them changes, is added or is removed.
include_guard(GLOBAL)
if(NOT DEFINED EXPANDED_GENERATED_DIR)
    set(EXPANDED_GENERATED_DIR ${CMAKE_BINARY_DIR}/expanded_generated)
endif()
file(GLOB_RECURSE expanded_data_builders CONFIGURE_DEPENDS
    ${EXPANDED_DIR}/src/data/*.cpp ${EXPANDED_DIR}/include/expanded/data/*.hpp)
list(SORT expanded_data_builders)
set(expanded_data_digest "")
foreach(builder IN LISTS expanded_data_builders)
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${builder})
    file(READ ${builder} builder_text)
    string(REPLACE "\r\n" "\n" builder_text "${builder_text}")
    string(SHA256 builder_hash "${builder_text}")
    file(RELATIVE_PATH builder_path ${EXPANDED_DIR} ${builder})
    string(APPEND expanded_data_digest "${builder_path} ${builder_hash}\n")
endforeach()
string(SHA256 expanded_data_version "${expanded_data_digest}")
string(SUBSTRING ${expanded_data_version} 0 16 expanded_data_version)
file(CONFIGURE OUTPUT ${EXPANDED_GENERATED_DIR}/expanded/data/data_version.hpp
    CONTENT "#pragma once\n#include <string_view>\nnamespace expanded {\ninline constexpr std::string_view kExpandedDataVersion = \"${expanded_data_version}\";\n}\n")
