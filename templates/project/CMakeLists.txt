cmake_minimum_required(VERSION 3.20)
project(__PROJECT_NAME__ LANGUAGES C)

# ---------------------------------------------------------------------------
# BridgeEngine 的两种引用方式：
#
#   A) 指定引擎源码目录（推荐）：把 BRIDGEENGINE_SOURCE_DIR 填成引擎源码路径
#      （或留空后在 cmake 命令行 -DBRIDGEENGINE_SOURCE_DIR=... 传入），直接
#      add_subdirectory，无需先安装引擎。
#   B) 使用已安装的引擎包：先执行 cmake --install build --prefix <前缀>，
#      再配置本工程时加 -DCMAKE_PREFIX_PATH=<前缀>。BRIDGEENGINE_SOURCE_DIR
#      为空时走 find_package。
# ---------------------------------------------------------------------------
set(BRIDGEENGINE_SOURCE_DIR "__ENGINE_DIR__")
if(NOT BRIDGEENGINE_SOURCE_DIR)
	find_package(BridgeEngine CONFIG REQUIRED)
	set(BRIDGEENGINE_LINK_TARGET BridgeEngine::bridgeengine_static)
else()
	# Only the engine static library is needed here; keep its optional
	# subprojects (editor, examples, tests, shared lib, XJ380) out of the build.
	set(BRIDGEENGINE_BUILD_SHARED OFF CACHE BOOL "" FORCE)
	set(BRIDGEENGINE_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
	set(BRIDGEENGINE_BUILD_TESTS OFF CACHE BOOL "" FORCE)
	set(BRIDGEENGINE_BUILD_EDITOR OFF CACHE BOOL "" FORCE)
	set(BRIDGEENGINE_BUILD_XJ380 OFF CACHE BOOL "" FORCE)
	add_subdirectory(${BRIDGEENGINE_SOURCE_DIR} bridgeengine)
	set(BRIDGEENGINE_LINK_TARGET bridgeengine_static)
endif()

add_executable(__EXEC_NAME__ main.c)
target_link_libraries(__EXEC_NAME__ PRIVATE ${BRIDGEENGINE_LINK_TARGET})

# 运行时需要 assets/ 与 ui/（默认字体与场景 xml），构建后自动复制到 exe 旁。
add_custom_command(TARGET __EXEC_NAME__ POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${CMAKE_CURRENT_SOURCE_DIR}/assets" "$<TARGET_FILE_DIR:__EXEC_NAME__>/assets"
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${CMAKE_CURRENT_SOURCE_DIR}/ui" "$<TARGET_FILE_DIR:__EXEC_NAME__>/ui"
)
