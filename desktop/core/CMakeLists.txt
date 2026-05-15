add_library(yeti-core STATIC
    ssh_deployer.h
    ssh_deployer.cpp
)

set_target_properties(yeti-core PROPERTIES
    AUTOMOC ON
)

target_include_directories(yeti-core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

find_package(libssh2 CONFIG REQUIRED)
target_link_libraries(yeti-core PUBLIC libssh2::libssh2)