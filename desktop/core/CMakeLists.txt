add_library(yeti-core STATIC
    ssh_deployer.h
    ssh_deployer.cpp
    config_manager.h
    config_manager.cpp
    http_client.h
    http_client.cpp
    client.h
    client.cpp
    log_buffer.h
    log_buffer.cpp
)

set_target_properties(yeti-core PROPERTIES
    AUTOMOC ON
)

target_include_directories(yeti-core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

find_package(libssh CONFIG REQUIRED)

target_link_libraries(yeti-core PUBLIC 
    ssh
    Qt6::Core
    Qt6::Network
    Qt6::WebSockets
)