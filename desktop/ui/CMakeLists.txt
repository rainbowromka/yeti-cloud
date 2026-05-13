add_executable(yeti-desktop
    main.cpp
    tray_icon.h
    tray_icon.cpp
    deploy_dialog.h
    deploy_dialog.cpp
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
