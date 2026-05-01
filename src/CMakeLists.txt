
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
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(WinposQt
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::WaylandClient
    PRIVATE
        Qt6::GuiPrivate
)
