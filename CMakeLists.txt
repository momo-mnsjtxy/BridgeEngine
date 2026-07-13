cmake_minimum_required(VERSION 3.20)
project(BridgeEngine VERSION 1.0.0 LANGUAGES C CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# ── Options ──────────────────────────────────────────────────────────────────
option(BUILD_SHARED_LIBS "Build BridgeEngine as a shared library" ON)
option(BUILD_EXAMPLES   "Build example executables"               ON)
option(ENABLE_XJ380     "Build for the XJ380 embedded target"     OFF)

# ── C/C++ Standard ───────────────────────────────────────────────────────────
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Visibility (shared libs on non-Windows) ──────────────────────────────────
set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

# ── Sources ──────────────────────────────────────────────────────────────────
set(ENGINE_CORE_SOURCES
    engine/platform/platform.c
    engine/master/init.c
    engine/master/version.c
    engine/render/draw.c
    engine/mouse_drawing.c
    engine/text/text.c
    engine/log/log.c
    engine/button/button.c
    engine/scene/scene.c
    engine/level/level.c
    engine/xml/xml_loader.c
    engine/audio/audio.c
    engine/math/math.c
    engine/texture/texture.c
    engine/input/input.c
    engine/camera/camera.c
)

set(ENGINE_PLATFORM_SDL3_SOURCES
    engine/platform/sdl3/platform_sdl3.c
    engine/video/video.c
)

set(ENGINE_PLATFORM_XJ380_SOURCES
    engine/platform/xj380/platform_xj380.cpp
    engine/video/video_stub.c
)

set(BRIDGEENGINE_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")

# ── Platform selection ───────────────────────────────────────────────────────
if(NOT ENABLE_XJ380)
    list(APPEND ENGINE_CORE_SOURCES ${ENGINE_PLATFORM_SDL3_SOURCES})
else()
    list(APPEND ENGINE_CORE_SOURCES ${ENGINE_PLATFORM_XJ380_SOURCES})
endif()

# ── Library target ───────────────────────────────────────────────────────────
add_library(BridgeEngine ${ENGINE_CORE_SOURCES})
target_include_directories(BridgeEngine PUBLIC ${BRIDGEENGINE_INCLUDE_DIR})
add_library(BridgeEngine::BridgeEngine ALIAS BridgeEngine)

# ── Compile definitions ──────────────────────────────────────────────────────
target_compile_definitions(BridgeEngine PRIVATE BAPI_LOG_ENABLED)

if(ENABLE_XJ380)
    target_compile_definitions(BridgeEngine PRIVATE
        USE_BACKEND_XJ380 __XJ380_OS__ XJ380_OS
        M_PI=3.14159265358979323846
        cosf=cos sinf=sin sqrtf=sqrt
        abs=__xj380_abs
    )
endif()

# ── Dependencies (PC / SDL3) ─────────────────────────────────────────────────
if(NOT ENABLE_XJ380)
    find_package(SDL3       REQUIRED)
    find_package(SDL3_image REQUIRED)
    find_package(SDL3_ttf   REQUIRED)
    find_package(FFmpeg     REQUIRED)

    target_link_libraries(BridgeEngine PRIVATE
        SDL3::SDL3
        SDL3_image::SDL3_image
        SDL3_ttf::SDL3_ttf
        FFmpeg::avcodec
        FFmpeg::avformat
        FFmpeg::avutil
        FFmpeg::swscale
        FFmpeg::swresample
    )

    if(WIN32)
        target_link_libraries(BridgeEngine PRIVATE opengl32 gdi32 user32 kernel32 shell32)
    else()
        target_link_libraries(BridgeEngine PRIVATE GL m)
    endif()
endif()

# ── XJ380-specific compile flags ─────────────────────────────────────────────
if(ENABLE_XJ380)
    target_compile_options(BridgeEngine PRIVATE
        -ffreestanding -fno-builtin -fno-stack-protector -fshort-wchar -nostdinc
    )
endif()

# ── Compiler warnings ────────────────────────────────────────────────────────
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang" AND NOT ENABLE_XJ380)
    target_compile_options(BridgeEngine PRIVATE
        -Wall -Wextra -Wpedantic -Wshadow -Wstrict-prototypes -Wold-style-definition
    )
endif()

# ── Examples ─────────────────────────────────────────────────────────────────
if(BUILD_EXAMPLES AND NOT ENABLE_XJ380)
    add_executable(bridgeengine_demo main.c)
    target_link_libraries(bridgeengine_demo PRIVATE BridgeEngine)

    add_executable(text_example text_example.c)
    target_link_libraries(text_example PRIVATE BridgeEngine)

    add_executable(log_test log_test.c)
    target_link_libraries(log_test PRIVATE BridgeEngine)

    install(TARGETS bridgeengine_demo text_example log_test
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

# ── XJ380 executable ─────────────────────────────────────────────────────────
if(ENABLE_XJ380)
    add_executable(bridgeengine_demo_xj380 xj380_main.c)
    target_link_libraries(bridgeengine_demo_xj380 PRIVATE BridgeEngine)
    target_link_options(bridgeengine_demo_xj380 PRIVATE -Ttext=0x200000)
endif()

# ── Install ──────────────────────────────────────────────────────────────────
include(GNUInstallDirs)

install(TARGETS BridgeEngine
    LIBRARY       DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE       DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME       DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h"
)
