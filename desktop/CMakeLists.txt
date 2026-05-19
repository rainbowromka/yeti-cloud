cmake_minimum_required(VERSION 3.20)
project(YetiDesktop VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# vcpkg toolchain
if(DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "vcpkg toolchain file")
endif()

find_package(Qt6 REQUIRED COMPONENTS Widgets Network WebSockets)

# ── Go server build (cross-compile to Linux) ──────────────────────────────
set(YETI_SERVER_SRC "${CMAKE_SOURCE_DIR}/../server")
set(YETI_SERVER_OUT "${CMAKE_BINARY_DIR}/ui/${CMAKE_CFG_INTDIR}/yeti-server")

add_custom_target(yeti-server-build ALL
    COMMAND ${CMAKE_COMMAND} -E env
        GOOS=linux GOARCH=amd64
        go build -o "${YETI_SERVER_OUT}" ./cmd/server
    WORKING_DIRECTORY "${YETI_SERVER_SRC}"
    COMMENT "Building Go server for Linux → ${YETI_SERVER_OUT}"
)

add_subdirectory(core)
add_subdirectory(ui)

add_dependencies(yeti-desktop yeti-server-build)