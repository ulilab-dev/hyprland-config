cmake_minimum_required(VERSION 3.20)
project(hyprbar CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ── Compiler flags ────────────────────────────────────────────────────────────
add_compile_options(-Wall -Wextra -O2 -pipe)

# ── Dependencies ──────────────────────────────────────────────────────────────
find_package(PkgConfig REQUIRED)
pkg_check_modules(WAYLAND   REQUIRED IMPORTED_TARGET wayland-client)
pkg_check_modules(CAIRO     REQUIRED IMPORTED_TARGET cairo)

find_program(WAYLAND_SCANNER NAMES wayland-scanner REQUIRED)

# ── Protocol generation ───────────────────────────────────────────────────────
set(PROTO_DIR     ${CMAKE_SOURCE_DIR}/protocols)
set(PROTO_OUT_DIR ${CMAKE_BINARY_DIR}/proto_gen)
file(MAKE_DIRECTORY ${PROTO_OUT_DIR})

function(wayland_protocol NAME XML)
    set(H   ${PROTO_OUT_DIR}/${NAME}-client-protocol.h)
    set(SRC ${PROTO_OUT_DIR}/${NAME}-protocol.c)
    add_custom_command(
        OUTPUT ${H}
        COMMAND ${WAYLAND_SCANNER} client-header ${XML} ${H}
        DEPENDS ${XML}
        COMMENT "wayland-scanner client-header ${NAME}")
    add_custom_command(
        OUTPUT ${SRC}
        COMMAND ${WAYLAND_SCANNER} private-code ${XML} ${SRC}
        DEPENDS ${XML}
        COMMENT "wayland-scanner private-code ${NAME}")
    set(PROTO_SOURCES ${PROTO_SOURCES} ${H} ${SRC} PARENT_SCOPE)
endfunction()

wayland_protocol(wlr-layer-shell-unstable-v1
    ${PROTO_DIR}/wlr-layer-shell-unstable-v1.xml)

# ── Sources ───────────────────────────────────────────────────────────────────
add_executable(hyprbar
    src/main.cpp
    src/Bar.cpp
    src/HyprIPC.cpp
    src/modules/Workspaces.cpp
    src/xdg_stub.c
    ${PROTO_SOURCES}
)

target_include_directories(hyprbar PRIVATE
    src
    ${PROTO_OUT_DIR}
)

target_link_libraries(hyprbar PRIVATE
    PkgConfig::WAYLAND
    PkgConfig::CAIRO
)

# ── Install ───────────────────────────────────────────────────────────────────
install(TARGETS hyprbar DESTINATION bin)
