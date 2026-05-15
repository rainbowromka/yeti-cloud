add_executable(yeti-desktop
    main.cpp
    tray_icon.h
    tray_icon.cpp
    main_window.h
    main_window.cpp
    add_server_page.h
    add_server_page.cpp
    status_page.h
    status_page.cpp
)

set_target_properties(yeti-desktop PROPERTIES
    AUTOMOC ON
)

target_link_libraries(yeti-desktop PRIVATE
    yeti-core
    Qt6::Widgets
    Qt6::Network
    Qt6::WebSockets
)