
add_library(WinposQt
    manager.cpp
    zone.cpp
    zoneitem.cpp
    windowpositioner.cpp

    Winpos/Manager.h
    Winpos/Zone.h
    Winpos/ZoneItem.h
    Winpos/WindowPositioner.h
)
add_library(Winpos::Qt ALIAS WinposQt)

set_target_properties(WinposQt PROPERTIES
    AUTOMOC ON
    VERSION ${PROJECT_VERSION}
    SOVERSION 0
    OUTPUT_NAME winpos-qt
    DEFINE_SYMBOL WINPOSQT_BUILD_LIBRARY
)

qt6_generate_wayland_protocol_client_sources(WinposQt
    PRIVATE_CODE
    FILES
        ${WaylandProtocols_DATADIR}/stable/xdg-shell/xdg-shell.xml
        ${CMAKE_SOURCE_DIR}/protocol/xx-zones-v1.xml
)

target_include_directories(WinposQt
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(WinposQt
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::WaylandClient
    PRIVATE
        Qt6::GuiPrivate
)

install(TARGETS WinposQt
    EXPORT WinposQtTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES
    Winpos/Manager.h
    Winpos/Zone.h
    Winpos/ZoneItem.h
    Winpos/WindowPositioner.h
    Winpos/winposqt_export.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/Winpos
)

#
# CMake package files
#
configure_package_config_file(
        ../cmake/WinposQtConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/WinposQtConfig.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/WinposQt
)

write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/WinposQtConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
)

install(EXPORT WinposQtTargets
        FILE WinposQtTargets.cmake
        NAMESPACE Winpos::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/WinposQt
)

install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/WinposQtConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/WinposQtConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/WinposQt
)
