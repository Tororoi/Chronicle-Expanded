# The mod's resources: the Confuse icon as a generated header, and expanded/lang beside the executable.
set(EXPANDED_GENERATED_DIR ${CMAKE_BINARY_DIR}/expanded_generated)
target_include_directories(dc_options INTERFACE ${EXPANDED_GENERATED_DIR})

# resources/confuse-20.rgba, the Confuse ability's 20x20 menu icon (confuse-20.png) as raw RGBA.
set(CONFUSE_ICON_RGBA ${EXPANDED_DIR}/resources/confuse-20.rgba)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${CONFUSE_ICON_RGBA})
file(READ ${CONFUSE_ICON_RGBA} confuse_icon_hex HEX)
string(REGEX REPLACE "(..)" "0x\\1," confuse_icon_bytes "${confuse_icon_hex}")
file(CONFIGURE OUTPUT ${EXPANDED_GENERATED_DIR}/expanded/status/confuse_icon.hpp
    CONTENT "#pragma once\n#include <cstdint>\ninline constexpr int kConfuseIconSize = 20;\ninline constexpr std::uint8_t kConfuseIconRgba[] = {${confuse_icon_bytes}};\n")

# The mod's strings (expanded/core/localize.hpp's kLanguageFolder), beside the port's: in the app's assets on Android.
if(ANDROID)
    file(COPY ${EXPANDED_DIR}/lang/ DESTINATION ${ANDROID_ASSETS}/lang/expanded)
else()
    add_custom_target(expanded_lang
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${EXPANDED_DIR}/lang $<TARGET_FILE_DIR:${darkcloud_target}>/lang/expanded
        COMMENT "Copying Chronicle-Expanded's strings")
    add_dependencies(${darkcloud_target} expanded_lang)
    install(DIRECTORY ${EXPANDED_DIR}/lang/ DESTINATION ${CMAKE_INSTALL_BINDIR}/lang/expanded)
endif()
