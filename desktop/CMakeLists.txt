cmake_minimum_required(VERSION 3.20)
project(YetiDesktop VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Fix parallel build PDB conflict
if(MSVC)
    add_compile_options(/FS)
endif()

# vcpkg toolchain
if(DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "vcpkg toolchain file")
endif()

find_package(Qt6 REQUIRED COMPONENTS Widgets Network WebSockets)

# ───── Go server build (cross-compile to Linux) ──────────────────────────────
set(YETI_SERVER_SRC "${CMAKE_SOURCE_DIR}/../server")
set(YETI_SERVER_OUT "${CMAKE_BINARY_DIR}/ui/${CMAKE_CFG_INTDIR}/yeti-server")

add_custom_target(yeti-server-build ALL
    COMMAND ${CMAKE_COMMAND} -E env
        GOOS=linux GOARCH=amd64
        go build -o "${YETI_SERVER_OUT}" ./cmd/server
    WORKING_DIRECTORY "${YETI_SERVER_SRC}"
    COMMENT "Building Go server for Linux → ${YETI_SERVER_OUT}"
)

# ───── Copy test config to build directory ───────────────────────────────────
set(YETI_DESKTOP_CONFIG_SRC "${CMAKE_SOURCE_DIR}/config/windows/yeti-desktop-config.json")
set(YETI_DESKTOP_CONFIG_DST "${CMAKE_BINARY_DIR}/ui/${CMAKE_CFG_INTDIR}/yeti-desktop-config.json")

add_custom_target(yeti-config-copy ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${YETI_DESKTOP_CONFIG_SRC}"
        "${YETI_DESKTOP_CONFIG_DST}"
    COMMENT "Copying desktop test config → ${YETI_DESKTOP_CONFIG_DST}"
)

# ───── Copy Go server binary to server/bin for Docker ────────────────────────
set(YETI_SERVER_DOCKER_DST "${CMAKE_SOURCE_DIR}/../server/bin/yeti-server")

add_custom_target(yeti-server-copy ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_SOURCE_DIR}/../server/bin"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${YETI_SERVER_OUT}"
        "${YETI_SERVER_DOCKER_DST}"
    COMMENT "Copying server binary → ${YETI_SERVER_DOCKER_DST}"
    DEPENDS yeti-server-build
)

add_subdirectory(core)
add_subdirectory(ui)

add_dependencies(yeti-desktop yeti-config-copy)