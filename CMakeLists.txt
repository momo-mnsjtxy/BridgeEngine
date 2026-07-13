cmake_minimum_required(VERSION 3.20)

project(BridgeEngine VERSION 0.1.0 LANGUAGES C CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "Generate compile_commands.json" FORCE)

option(BRIDGEENGINE_BUILD_SHARED "Build the shared BridgeEngine library" ON)
option(BRIDGEENGINE_BUILD_STATIC "Build the static BridgeEngine library" ON)
option(BRIDGEENGINE_BUILD_EXAMPLES "Build BridgeEngine example executables" ON)
option(BRIDGEENGINE_BUILD_XJ380 "Enable XJ380 custom build targets" ON)
option(BRIDGEENGINE_REQUIRE_DESKTOP_DEPS "Require SDL3, SDL3_image, SDL3_ttf, and FFmpeg at configure time" OFF)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(BRIDGEENGINE_PLATFORM_SOURCES
	engine/platform/platform.c
	engine/platform/sdl3/platform_sdl3.c
)
set(BRIDGEENGINE_CORE_SOURCES
	engine/master/state.c
	engine/master/init.c
	engine/master/version.c
	engine/log/log.c
)
set(BRIDGEENGINE_RENDER_SOURCES
	engine/render/draw.c
	engine/mouse_drawing.c
	engine/text/text.c
	engine/math/math.c
	engine/texture/texture.c
	engine/input/input.c
	engine/camera/camera.c
)
set(BRIDGEENGINE_UI_SOURCES
	engine/button/button.c
	engine/scene/scene.c
	engine/level/level.c
	engine/xml/xml_loader.c
)
set(BRIDGEENGINE_MEDIA_SOURCES
	engine/audio/audio.c
	engine/video/video.c
)
set(BRIDGEENGINE_SOURCES
	${BRIDGEENGINE_PLATFORM_SOURCES}
	${BRIDGEENGINE_CORE_SOURCES}
	${BRIDGEENGINE_RENDER_SOURCES}
	${BRIDGEENGINE_UI_SOURCES}
	${BRIDGEENGINE_MEDIA_SOURCES}
)

if(BRIDGEENGINE_BUILD_SHARED OR BRIDGEENGINE_BUILD_STATIC OR BRIDGEENGINE_BUILD_EXAMPLES)
	set(BRIDGEENGINE_DESKTOP_DEPS_FOUND TRUE)
	find_package(PkgConfig QUIET)

	if(WIN32)
		if(BRIDGEENGINE_REQUIRE_DESKTOP_DEPS)
			find_package(SDL3 CONFIG REQUIRED)
			find_package(SDL3_image CONFIG REQUIRED)
			find_package(SDL3_ttf CONFIG REQUIRED)
			find_package(FFmpeg REQUIRED)
		else()
			find_package(SDL3 CONFIG QUIET)
			find_package(SDL3_image CONFIG QUIET)
			find_package(SDL3_ttf CONFIG QUIET)
			find_package(FFmpeg QUIET)
		endif()
		if(NOT TARGET SDL3::SDL3 OR NOT TARGET SDL3_image::SDL3_image OR NOT TARGET SDL3_ttf::SDL3_ttf OR NOT FFmpeg_FOUND)
			set(BRIDGEENGINE_DESKTOP_DEPS_FOUND FALSE)
		endif()

		set(BRIDGEENGINE_PLATFORM_LIBS
			$<$<TARGET_EXISTS:SDL3::SDL3>:SDL3::SDL3>
			$<$<TARGET_EXISTS:SDL3_image::SDL3_image>:SDL3_image::SDL3_image>
			$<$<TARGET_EXISTS:SDL3_ttf::SDL3_ttf>:SDL3_ttf::SDL3_ttf>
			$<$<TARGET_EXISTS:FFmpeg::avcodec>:FFmpeg::avcodec>
			$<$<TARGET_EXISTS:FFmpeg::avformat>:FFmpeg::avformat>
			$<$<TARGET_EXISTS:FFmpeg::avutil>:FFmpeg::avutil>
			$<$<TARGET_EXISTS:FFmpeg::swscale>:FFmpeg::swscale>
			$<$<TARGET_EXISTS:FFmpeg::swresample>:FFmpeg::swresample>
			opengl32 gdi32 user32 kernel32 shell32
		)
		set(BRIDGEENGINE_PLATFORM_INCLUDES ${FFmpeg_INCLUDE_DIRS})
	else()
		if(BRIDGEENGINE_REQUIRE_DESKTOP_DEPS)
			set(BRIDGEENGINE_PKG_REQUIRED REQUIRED)
		else()
			set(BRIDGEENGINE_PKG_REQUIRED)
		endif()
		pkg_check_modules(SDL3 ${BRIDGEENGINE_PKG_REQUIRED} sdl3)
		pkg_check_modules(SDL3_IMAGE ${BRIDGEENGINE_PKG_REQUIRED} sdl3-image)
		pkg_check_modules(SDL3_TTF ${BRIDGEENGINE_PKG_REQUIRED} sdl3-ttf)
		pkg_check_modules(FFMPEG ${BRIDGEENGINE_PKG_REQUIRED} libavcodec libavformat libavutil libswscale libswresample)
		if(NOT SDL3_FOUND OR NOT SDL3_IMAGE_FOUND OR NOT SDL3_TTF_FOUND OR NOT FFMPEG_FOUND)
			set(BRIDGEENGINE_DESKTOP_DEPS_FOUND FALSE)
		endif()

		if(NOT SDL3_LIBRARIES)
			set(SDL3_LIBRARIES SDL3)
		endif()
		if(NOT SDL3_IMAGE_LIBRARIES)
			set(SDL3_IMAGE_LIBRARIES SDL3_image)
		endif()
		if(NOT SDL3_TTF_LIBRARIES)
			set(SDL3_TTF_LIBRARIES SDL3_ttf)
		endif()
		if(NOT FFMPEG_LIBRARIES)
			set(FFMPEG_LIBRARIES avcodec avformat avutil swscale swresample)
		endif()

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
	if(NOT BRIDGEENGINE_DESKTOP_DEPS_FOUND AND NOT BRIDGEENGINE_REQUIRE_DESKTOP_DEPS)
		message(WARNING "Desktop SDL/FFmpeg dependencies are incomplete. Desktop library targets are generated for compile_commands.json but excluded from the default build. Install SDL3_image and SDL3_ttf development packages, or configure with -DBRIDGEENGINE_REQUIRE_DESKTOP_DEPS=ON to fail early.")
	endif()
endif()

function(bridgeengine_configure_target target_name)
	target_include_directories(${target_name}
		PUBLIC
			$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
			$<INSTALL_INTERFACE:include>
		PRIVATE
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
	foreach(asset_dir IN ITEMS audio_example video_example image_example text)
		if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${asset_dir}")
			add_custom_command(TARGET ${target_name} POST_BUILD
				COMMAND ${CMAKE_COMMAND} -E copy_directory
					"${CMAKE_CURRENT_SOURCE_DIR}/${asset_dir}"
					"$<TARGET_FILE_DIR:${target_name}>/${asset_dir}"
			)
		endif()
	endforeach()
endfunction()

set(BRIDGEENGINE_DESKTOP_TARGET_SCOPE)
if(DEFINED BRIDGEENGINE_DESKTOP_DEPS_FOUND AND NOT BRIDGEENGINE_DESKTOP_DEPS_FOUND)
	set(BRIDGEENGINE_DESKTOP_TARGET_SCOPE EXCLUDE_FROM_ALL)
endif()

if(BRIDGEENGINE_BUILD_SHARED)
	add_library(bridgeengine_shared SHARED ${BRIDGEENGINE_DESKTOP_TARGET_SCOPE} ${BRIDGEENGINE_SOURCES})
	set_target_properties(bridgeengine_shared PROPERTIES OUTPUT_NAME bridgeengine)
	bridgeengine_configure_target(bridgeengine_shared)
	add_library(BridgeEngine::shared ALIAS bridgeengine_shared)
endif()

if(BRIDGEENGINE_BUILD_STATIC)
	add_library(bridgeengine_static STATIC ${BRIDGEENGINE_DESKTOP_TARGET_SCOPE} ${BRIDGEENGINE_SOURCES})
	set_target_properties(bridgeengine_static PROPERTIES OUTPUT_NAME bridgeengine)
	bridgeengine_configure_target(bridgeengine_static)
	add_library(BridgeEngine::static ALIAS bridgeengine_static)
endif()

if(BRIDGEENGINE_BUILD_EXAMPLES AND (NOT DEFINED BRIDGEENGINE_DESKTOP_DEPS_FOUND OR BRIDGEENGINE_DESKTOP_DEPS_FOUND))
	set(BRIDGEENGINE_EXAMPLE_LINK_TARGET bridgeengine_static)
	if(NOT TARGET bridgeengine_static)
		set(BRIDGEENGINE_EXAMPLE_LINK_TARGET bridgeengine_shared)
	endif()

	add_executable(main main.c)
	target_link_libraries(main PRIVATE ${BRIDGEENGINE_EXAMPLE_LINK_TARGET})
	bridgeengine_copy_runtime_assets(main)

	add_executable(text_example text_example.c)
	target_link_libraries(text_example PRIVATE ${BRIDGEENGINE_EXAMPLE_LINK_TARGET})
	bridgeengine_copy_runtime_assets(text_example)

	add_executable(log_test log_test.c)
	target_link_libraries(log_test PRIVATE ${BRIDGEENGINE_EXAMPLE_LINK_TARGET})
	bridgeengine_copy_runtime_assets(log_test)
elseif(BRIDGEENGINE_BUILD_EXAMPLES)
	message(WARNING "Skipping desktop examples because SDL/FFmpeg dependencies are incomplete.")
endif()

if(BRIDGEENGINE_BUILD_XJ380)
	set(XJ380_SDK "${CMAKE_CURRENT_SOURCE_DIR}/XJ380_XACT_2026v4_xj380/depend" CACHE PATH "Path to the XJ380 SDK depend directory")
	set(XJ380_CC clang CACHE FILEPATH "XJ380 C compiler")
	set(XJ380_CXX clang++ CACHE FILEPATH "XJ380 C++ compiler")
	set(XJ380_LD ld CACHE FILEPATH "XJ380 linker")

	set(XJ380_C_SOURCES
		engine/platform/platform.c
		engine/master/state.c
		engine/master/init.c
		engine/master/version.c
		engine/log/log.c
		engine/render/draw.c
		engine/mouse_drawing.c
		engine/text/text.c
		engine/math/math.c
		engine/texture/texture.c
		engine/input/input.c
		engine/camera/camera.c
		engine/button/button.c
		engine/scene/scene.c
		engine/level/level.c
		engine/xml/xml_loader.c
		engine/audio/audio.c
		engine/video/video_stub.c
	)
	set(XJ380_CPP_SOURCES engine/platform/xj380/platform_xj380.cpp)
	set(XJ380_OBJECTS)

	set(XJ380_COMMON_FLAGS
		-ffreestanding -fno-builtin -m64 -fno-stack-protector -fshort-wchar
		-nostdinc -O -g -fPIC
		-I${CMAKE_CURRENT_SOURCE_DIR}/include
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
		COMMAND ${XJ380_CC} ${XJ380_COMMON_FLAGS} -std=c11 -c -o "${CMAKE_CURRENT_BINARY_DIR}/xj380/xj380_main.xj380.o" "${CMAKE_CURRENT_SOURCE_DIR}/xj380_main.c"
		DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/xj380_main.c" check_xj380_sdk
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
install(DIRECTORY include/ DESTINATION include)
