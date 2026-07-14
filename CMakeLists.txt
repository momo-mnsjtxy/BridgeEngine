cmake_minimum_required(VERSION 3.20)

project(BridgeEngine VERSION 2.0.0 LANGUAGES C CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "Generate compile_commands.json" FORCE)

option(BRIDGEENGINE_BUILD_SHARED "Build the shared BridgeEngine library" ON)
option(BRIDGEENGINE_BUILD_STATIC "Build the static BridgeEngine library" ON)
option(BRIDGEENGINE_BUILD_EXAMPLES "Build BridgeEngine example executables" ON)
option(BRIDGEENGINE_BUILD_XJ380 "Enable XJ380 custom build targets" ON)
option(BRIDGEENGINE_BUILD_TESTS "Build BridgeEngine regression tests" ON)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake/bridgeengine_version.h.in"
	"${CMAKE_CURRENT_BINARY_DIR}/generated/bridgeengine_version.h"
	@ONLY
)

set(BRIDGEENGINE_PLATFORM_SOURCES
	src/platform/platform.c
	src/platform/sdl3/platform_sdl3.c
)
set(BRIDGEENGINE_CORE_SOURCES
	src/core/render_context.c
	src/core/init.c
	src/core/version.c
	src/log.c
)
set(BRIDGEENGINE_RENDER_SOURCES
	src/draw.c
	src/input/mouse_drawing.c
	src/text.c
	src/math.c
	src/texture.c
	src/input/input.c
	src/camera.c
)
set(BRIDGEENGINE_UI_SOURCES
	src/button.c
	src/scene.c
	src/level.c
	src/xml_loader.c
)
set(BRIDGEENGINE_MEDIA_SOURCES
	src/audio.c
	src/video/video.c
)
set(BRIDGEENGINE_SOURCES
	${BRIDGEENGINE_PLATFORM_SOURCES}
	${BRIDGEENGINE_CORE_SOURCES}
	${BRIDGEENGINE_RENDER_SOURCES}
	${BRIDGEENGINE_UI_SOURCES}
	${BRIDGEENGINE_MEDIA_SOURCES}
)

if(BRIDGEENGINE_BUILD_SHARED OR BRIDGEENGINE_BUILD_STATIC OR BRIDGEENGINE_BUILD_EXAMPLES)
	find_package(PkgConfig QUIET)

	if(WIN32)
		find_package(SDL3 CONFIG REQUIRED)
		find_package(SDL3_image CONFIG REQUIRED)
		find_package(SDL3_ttf CONFIG REQUIRED)
		find_package(FFmpeg REQUIRED)

		set(BRIDGEENGINE_PLATFORM_LIBS
			SDL3::SDL3 SDL3_image::SDL3_image SDL3_ttf::SDL3_ttf
			FFmpeg::avcodec FFmpeg::avformat FFmpeg::avutil FFmpeg::swscale FFmpeg::swresample
			opengl32 gdi32 user32 kernel32 shell32
		)
		set(BRIDGEENGINE_PLATFORM_INCLUDES ${FFmpeg_INCLUDE_DIRS})
	else()
		pkg_check_modules(SDL3 REQUIRED sdl3)
		pkg_check_modules(SDL3_IMAGE REQUIRED sdl3-image)
		pkg_check_modules(SDL3_TTF REQUIRED sdl3-ttf)
		pkg_check_modules(FFMPEG REQUIRED libavcodec libavformat libavutil libswscale libswresample)

		set(BRIDGEENGINE_PLATFORM_LIBS
			${SDL3_LIBRARIES}
			${SDL3_IMAGE_LIBRARIES}
			${SDL3_TTF_LIBRARIES}
			${FFMPEG_LIBRARIES}
			GL
			m
		)
		set(BRIDGEENGINE_PLATFORM_INCLUDES
			${SDL3_INCLUDE_DIRS}
			${SDL3_IMAGE_INCLUDE_DIRS}
			${SDL3_TTF_INCLUDE_DIRS}
			${FFMPEG_INCLUDE_DIRS}
		)
		set(BRIDGEENGINE_PLATFORM_LINK_DIRS
			${SDL3_LIBRARY_DIRS}
			${SDL3_IMAGE_LIBRARY_DIRS}
			${SDL3_TTF_LIBRARY_DIRS}
			${FFMPEG_LIBRARY_DIRS}
		)
	endif()
endif()

function(bridgeengine_configure_target target_name)
	target_include_directories(${target_name}
		PUBLIC
			$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
			$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated>
			$<INSTALL_INTERFACE:include>
		PRIVATE
			${CMAKE_CURRENT_SOURCE_DIR}/src
			${BRIDGEENGINE_PLATFORM_INCLUDES}
	)
	target_compile_definitions(${target_name} PRIVATE BAPI_LOG_ENABLED)
	target_compile_options(${target_name} PRIVATE
		$<$<COMPILE_LANGUAGE:C>:-Wall -Wextra>
		$<$<COMPILE_LANGUAGE:CXX>:-Wall -Wextra>
	)
	if(BRIDGEENGINE_PLATFORM_LINK_DIRS)
		target_link_directories(${target_name} PRIVATE ${BRIDGEENGINE_PLATFORM_LINK_DIRS})
	endif()
	target_link_libraries(${target_name} PRIVATE ${BRIDGEENGINE_PLATFORM_LIBS})
endfunction()

function(bridgeengine_copy_runtime_assets target_name)
	if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples/assets")
		add_custom_command(TARGET ${target_name} POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_directory
				"${CMAKE_CURRENT_SOURCE_DIR}/examples/assets"
				"$<TARGET_FILE_DIR:${target_name}>/assets"
		)
	endif()
endfunction()

if(BRIDGEENGINE_BUILD_TESTS)
	enable_testing()
	add_executable(bridgeengine_public_header_c_test tests/public_header_c_test.c)
	target_include_directories(bridgeengine_public_header_c_test PRIVATE
		"${CMAKE_CURRENT_SOURCE_DIR}/include"
		"${CMAKE_CURRENT_BINARY_DIR}/generated"
	)
	add_test(NAME bridgeengine.public_header_c COMMAND bridgeengine_public_header_c_test)

	add_executable(bridgeengine_public_header_cpp_test tests/public_header_cpp_test.cpp)
	target_include_directories(bridgeengine_public_header_cpp_test PRIVATE
		"${CMAKE_CURRENT_SOURCE_DIR}/include"
		"${CMAKE_CURRENT_BINARY_DIR}/generated"
	)
	add_test(NAME bridgeengine.public_header_cpp COMMAND bridgeengine_public_header_cpp_test)

	add_executable(bridgeengine_scene_level_xml_test
		tests/scene_level_xml_test.c
		src/scene.c
		src/level.c
		src/xml_loader.c
	)
	target_include_directories(bridgeengine_scene_level_xml_test PRIVATE
		"${CMAKE_CURRENT_SOURCE_DIR}/include"
		"${CMAKE_CURRENT_BINARY_DIR}/generated"
		"${CMAKE_CURRENT_SOURCE_DIR}/src"
	)
	add_test(NAME bridgeengine.scene_level_xml
		COMMAND bridgeengine_scene_level_xml_test "${CMAKE_CURRENT_BINARY_DIR}"
	)

	add_executable(bridgeengine_texture_test
		tests/texture_test.c
		src/texture.c
		src/platform/platform.c
	)
	target_include_directories(bridgeengine_texture_test PRIVATE
		"${CMAKE_CURRENT_SOURCE_DIR}/include"
		"${CMAKE_CURRENT_BINARY_DIR}/generated"
		"${CMAKE_CURRENT_SOURCE_DIR}/src"
	)
	add_test(NAME bridgeengine.texture COMMAND bridgeengine_texture_test)

	add_executable(bridgeengine_lifecycle_test
		tests/lifecycle_test.c
		src/core/render_context.c
		src/platform/platform.c
	)
	target_include_directories(bridgeengine_lifecycle_test PRIVATE
		"${CMAKE_CURRENT_SOURCE_DIR}/include"
		"${CMAKE_CURRENT_BINARY_DIR}/generated"
		"${CMAKE_CURRENT_SOURCE_DIR}/src"
	)
	add_test(NAME bridgeengine.lifecycle COMMAND bridgeengine_lifecycle_test)

	add_executable(bridgeengine_text_asset_test
		tests/text_asset_test.c
		src/text.c
		src/platform/platform.c
	)
	target_include_directories(bridgeengine_text_asset_test PRIVATE
		"${CMAKE_CURRENT_SOURCE_DIR}/include"
		"${CMAKE_CURRENT_BINARY_DIR}/generated"
		"${CMAKE_CURRENT_SOURCE_DIR}/src"
	)
	add_test(NAME bridgeengine.text_assets COMMAND bridgeengine_text_asset_test)
endif()

if(BRIDGEENGINE_BUILD_SHARED)
	add_library(bridgeengine_shared SHARED ${BRIDGEENGINE_SOURCES})
	set_target_properties(bridgeengine_shared PROPERTIES OUTPUT_NAME bridgeengine)
	bridgeengine_configure_target(bridgeengine_shared)
	add_library(BridgeEngine::shared ALIAS bridgeengine_shared)
endif()

if(BRIDGEENGINE_BUILD_STATIC)
	add_library(bridgeengine_static STATIC ${BRIDGEENGINE_SOURCES})
	set_target_properties(bridgeengine_static PROPERTIES OUTPUT_NAME bridgeengine)
	bridgeengine_configure_target(bridgeengine_static)
	add_library(BridgeEngine::static ALIAS bridgeengine_static)
endif()

if(BRIDGEENGINE_BUILD_EXAMPLES)
	set(BRIDGEENGINE_EXAMPLE_LINK_TARGET bridgeengine_static)
	if(NOT TARGET bridgeengine_static)
		set(BRIDGEENGINE_EXAMPLE_LINK_TARGET bridgeengine_shared)
	endif()

	add_executable(bridgeengine_desktop_example examples/desktop/main.c)
	target_link_libraries(bridgeengine_desktop_example PRIVATE ${BRIDGEENGINE_EXAMPLE_LINK_TARGET})
	bridgeengine_copy_runtime_assets(bridgeengine_desktop_example)

	add_executable(bridgeengine_text_example examples/text/text_example.c)
	target_link_libraries(bridgeengine_text_example PRIVATE ${BRIDGEENGINE_EXAMPLE_LINK_TARGET})
	bridgeengine_copy_runtime_assets(bridgeengine_text_example)

	add_executable(bridgeengine_log_example examples/log/log_test.c)
	target_link_libraries(bridgeengine_log_example PRIVATE ${BRIDGEENGINE_EXAMPLE_LINK_TARGET})
	bridgeengine_copy_runtime_assets(bridgeengine_log_example)
endif()

if(BRIDGEENGINE_BUILD_XJ380)
	set(XJ380_SDK "${CMAKE_CURRENT_SOURCE_DIR}/XJ380_XACT_2026v4_xj380/depend" CACHE PATH "Path to the XJ380 SDK depend directory")
	set(XJ380_CC clang CACHE FILEPATH "XJ380 C compiler")
	set(XJ380_CXX clang++ CACHE FILEPATH "XJ380 C++ compiler")
	set(XJ380_LD ld CACHE FILEPATH "XJ380 linker")

	set(XJ380_C_SOURCES
		src/platform/platform.c
		src/core/render_context.c
		src/core/init.c
		src/core/version.c
		src/log.c
		src/draw.c
		src/input/mouse_drawing.c
		src/text.c
		src/math.c
		src/texture.c
		src/input/input.c
		src/camera.c
		src/button.c
		src/scene.c
		src/level.c
		src/xml_loader.c
		src/audio.c
		src/video/video_stub.c
	)
	set(XJ380_CPP_SOURCES src/platform/xj380/platform_xj380.cpp)
	set(XJ380_OBJECTS)

	set(XJ380_COMMON_FLAGS
		-ffreestanding -fno-builtin -m64 -fno-stack-protector -fshort-wchar
		-nostdinc -O -g -fPIC
		-I${CMAKE_CURRENT_SOURCE_DIR}/include
		-I${CMAKE_CURRENT_SOURCE_DIR}/src
		-I${XJ380_SDK}/include
		-I${XJ380_SDK}/include/xposix
		-DUSE_BACKEND_XJ380 -DBAPI_LOG_ENABLED -D__XJ380_OS__ -DXJ380_OS
		-Dcosf=cos -Dsinf=sin -Dsqrtf=sqrt
		-Dabs=__xj380_abs
	)

	add_custom_target(check_xj380_sdk
		COMMAND ${CMAKE_COMMAND} -DXJ380_SDK=${XJ380_SDK} -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/CheckXJ380Sdk.cmake
	)

	foreach(source IN LISTS XJ380_C_SOURCES)
		set(object "${CMAKE_CURRENT_BINARY_DIR}/xj380/${source}.xj380.o")
		get_filename_component(object_dir "${object}" DIRECTORY)
		add_custom_command(
			OUTPUT "${object}"
			COMMAND ${CMAKE_COMMAND} -E make_directory "${object_dir}"
			COMMAND ${XJ380_CC} ${XJ380_COMMON_FLAGS} -std=c11 -c -o "${object}" "${CMAKE_CURRENT_SOURCE_DIR}/${source}"
			DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${source}" check_xj380_sdk
			VERBATIM
		)
		list(APPEND XJ380_OBJECTS "${object}")
	endforeach()

	foreach(source IN LISTS XJ380_CPP_SOURCES)
		set(object "${CMAKE_CURRENT_BINARY_DIR}/xj380/${source}.xj380.o")
		get_filename_component(object_dir "${object}" DIRECTORY)
		add_custom_command(
			OUTPUT "${object}"
			COMMAND ${CMAKE_COMMAND} -E make_directory "${object_dir}"
			COMMAND ${XJ380_CXX} ${XJ380_COMMON_FLAGS} -std=c++11 -fno-exceptions -c -o "${object}" "${CMAKE_CURRENT_SOURCE_DIR}/${source}"
			DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${source}" check_xj380_sdk
			VERBATIM
		)
		list(APPEND XJ380_OBJECTS "${object}")
	endforeach()

	add_custom_command(
		OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/libbridgeengine_xj380.a"
		COMMAND ${CMAKE_AR} cr "${CMAKE_CURRENT_BINARY_DIR}/libbridgeengine_xj380.a" ${XJ380_OBJECTS}
		DEPENDS ${XJ380_OBJECTS}
		VERBATIM
	)
	add_custom_target(xj380_staticlib DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/libbridgeengine_xj380.a")

	file(GLOB_RECURSE XJ380_XAPI_OBJS
		"${XJ380_SDK}/obj-gui/*.o"
		"${XJ380_SDK}/obj-gui/*/*.o"
		"${XJ380_SDK}/obj-gui/*/*/*.o"
	)
	list(APPEND XJ380_XAPI_OBJS "${XJ380_SDK}/obj-gui/liballoc-x86_64.a")

	add_custom_command(
		OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/xj380/xj380_main.xj380.o"
		COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/xj380"
		COMMAND ${XJ380_CC} ${XJ380_COMMON_FLAGS} -std=c11 -c -o "${CMAKE_CURRENT_BINARY_DIR}/xj380/xj380_main.xj380.o" "${CMAKE_CURRENT_SOURCE_DIR}/examples/xj380/xj380_main.c"
		DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/examples/xj380/xj380_main.c" check_xj380_sdk
		VERBATIM
	)
	add_custom_command(
		OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/bridgeengine_demo.epf"
		COMMAND ${XJ380_LD} -Ttext=0x200000 ${XJ380_XAPI_OBJS} "${CMAKE_CURRENT_BINARY_DIR}/xj380/xj380_main.xj380.o" "${CMAKE_CURRENT_BINARY_DIR}/libbridgeengine_xj380.a" -o "${CMAKE_CURRENT_BINARY_DIR}/bridgeengine_demo.epf"
		DEPENDS xj380_staticlib "${CMAKE_CURRENT_BINARY_DIR}/xj380/xj380_main.xj380.o"
		VERBATIM
	)
	add_custom_target(xj380_epf DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/bridgeengine_demo.epf")
endif()

set(BRIDGEENGINE_INSTALL_TARGETS)
if(TARGET bridgeengine_shared)
	list(APPEND BRIDGEENGINE_INSTALL_TARGETS bridgeengine_shared)
endif()
if(TARGET bridgeengine_static)
	list(APPEND BRIDGEENGINE_INSTALL_TARGETS bridgeengine_static)
endif()
if(BRIDGEENGINE_INSTALL_TARGETS)
	install(TARGETS ${BRIDGEENGINE_INSTALL_TARGETS}
		EXPORT BridgeEngineTargets
		ARCHIVE DESTINATION lib
		LIBRARY DESTINATION lib
		RUNTIME DESTINATION bin
	)
endif()
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/include/BridgeEngine.h" DESTINATION include)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/generated/bridgeengine_version.h" DESTINATION include)
