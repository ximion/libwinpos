
find_package(Qt6 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS Widgets)

add_executable(winpos-demo
    main.cpp
    demowindow.h
    demowindow.cpp
)

set_target_properties(winpos-demo PROPERTIES AUTOMOC ON)
target_link_libraries(winpos-demo
    PRIVATE
        Winpos::Qt
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
)
