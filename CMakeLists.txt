cmake_minimum_required(VERSION 3.16)

project(LibWinpos VERSION "0.1.0")

set(CMAKE_C_STANDARD 23)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#
# Options
#
option(BUILD_EXAMPLES "Build Qt examples" ON)

#
# Dependencies
#
include(FeatureSummary)
find_package(PkgConfig REQUIRED)

set(QT_MIN_VERSION "6.8.2")

# Wayland protocol data directory
pkg_get_variable(WaylandProtocols_DATADIR wayland-protocols pkgdatadir)
if(NOT WaylandProtocols_DATADIR)
    message(FATAL_ERROR "wayland-protocols not found - install wayland-protocols")
endif()

add_feature_info(WaylandProtocols TRUE
    "Wayland protocol extensions data dir: ${WaylandProtocols_DATADIR}")

# Qt
set(QT_NO_PRIVATE_MODULE_WARNING ON)
find_package(Qt6 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS
        Core
        Gui
        GuiPrivate
        WaylandClient
)

add_subdirectory("src/")
if(BUILD_EXAMPLES)
    add_subdirectory("examples/")
endif()

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
