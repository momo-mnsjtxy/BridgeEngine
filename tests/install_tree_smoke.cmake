if(NOT DEFINED BUILD_DIR OR NOT DEFINED SOURCE_DIR)
  message(FATAL_ERROR "BUILD_DIR and SOURCE_DIR are required")
endif()

get_filename_component(BUILD_DIR "${BUILD_DIR}" ABSOLUTE)
get_filename_component(SOURCE_DIR "${SOURCE_DIR}" ABSOLUTE)
set(prefix "${BUILD_DIR}/install-smoke")
file(REMOVE_RECURSE "${prefix}")
set(install_command "${CMAKE_COMMAND}" --install "${BUILD_DIR}")
if(DEFINED INSTALL_CONFIG)
  list(APPEND install_command --config "${INSTALL_CONFIG}")
endif()
list(APPEND install_command --prefix "${prefix}")
execute_process(COMMAND ${install_command}
  RESULT_VARIABLE install_result)
if(NOT install_result EQUAL 0)
  message(FATAL_ERROR "installation failed")
endif()

set(configure_command
  "${CMAKE_COMMAND}" -S "${SOURCE_DIR}/tests/package_consumer"
  -B "${prefix}/package-consumer" "-DCMAKE_PREFIX_PATH=${prefix}")
if(DEFINED TOOLCHAIN_FILE)
  list(APPEND configure_command "-DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN_FILE}")
endif()
if(CONSUMER_DEPENDENCY_MODE STREQUAL "shared-only")
	list(APPEND configure_command -DEXPECT_SHARED=ON -DEXPECT_STATIC=OFF)
  list(APPEND configure_command
    -DCMAKE_DISABLE_FIND_PACKAGE_PkgConfig=TRUE
    -DCMAKE_DISABLE_FIND_PACKAGE_SDL3=TRUE
    -DCMAKE_DISABLE_FIND_PACKAGE_SDL3_image=TRUE
    -DCMAKE_DISABLE_FIND_PACKAGE_SDL3_ttf=TRUE
    -DCMAKE_DISABLE_FIND_PACKAGE_FFmpeg=TRUE)
elseif(CONSUMER_DEPENDENCY_MODE STREQUAL "static-only" AND NOT WIN32)
	list(APPEND configure_command -DEXPECT_SHARED=OFF -DEXPECT_STATIC=ON)
  list(APPEND configure_command
	-DCMAKE_DISABLE_FIND_PACKAGE_SDL3=TRUE
	-DCMAKE_DISABLE_FIND_PACKAGE_SDL3_image=TRUE
	-DCMAKE_DISABLE_FIND_PACKAGE_SDL3_ttf=TRUE)
elseif(CONSUMER_DEPENDENCY_MODE STREQUAL "static-only")
	list(APPEND configure_command -DEXPECT_SHARED=OFF -DEXPECT_STATIC=ON)
elseif(CONSUMER_DEPENDENCY_MODE STREQUAL "shared-static")
	list(APPEND configure_command -DEXPECT_SHARED=ON -DEXPECT_STATIC=ON)
else()
	message(FATAL_ERROR "CONSUMER_DEPENDENCY_MODE must select a package mode")
endif()
execute_process(COMMAND ${configure_command}
  RESULT_VARIABLE package_configure_result)
if(NOT package_configure_result EQUAL 0)
  message(FATAL_ERROR "find_package consumer configuration failed")
endif()
execute_process(COMMAND "${CMAKE_COMMAND}" --build "${prefix}/package-consumer" --config Release
  RESULT_VARIABLE package_build_result)
if(NOT package_build_result EQUAL 0)
  message(FATAL_ERROR "find_package consumer build failed")
endif()

set(test_command "${CMAKE_COMMAND}" -E env)
if(WIN32)
  set(runtime_path "${prefix}/bin\\;${prefix}/lib")
  if(DEFINED VCPKG_BIN_DIR)
    string(APPEND runtime_path "\\;${VCPKG_BIN_DIR}")
  endif()
  list(APPEND test_command "PATH=${runtime_path}\\;$ENV{PATH}")
elseif(APPLE)
  list(APPEND test_command "DYLD_LIBRARY_PATH=${prefix}/lib:$ENV{DYLD_LIBRARY_PATH}")
else()
  list(APPEND test_command "LD_LIBRARY_PATH=${prefix}/lib:$ENV{LD_LIBRARY_PATH}")
endif()
list(APPEND test_command "${CMAKE_CTEST_COMMAND}" --test-dir "${prefix}/package-consumer" --build-config Release --output-on-failure)
execute_process(COMMAND ${test_command}
  RESULT_VARIABLE package_test_result)
if(NOT package_test_result EQUAL 0)
  message(FATAL_ERROR "find_package consumer tests failed")
endif()

if((CONSUMER_DEPENDENCY_MODE STREQUAL "static-only" OR CONSUMER_DEPENDENCY_MODE STREQUAL "shared-static") AND NOT WIN32)
  set(pkgconfig_fixture_dir "${SOURCE_DIR}/tests/pkgconfig_static_fixture/pkgconfig")
  foreach(fixture_mode IN ITEMS default existing)
    set(fixture_command
      "${CMAKE_COMMAND}" -E env
      "PKG_CONFIG_LIBDIR=${pkgconfig_fixture_dir}"
      "PKG_CONFIG_PATH="
      "${CMAKE_COMMAND}" -S "${SOURCE_DIR}/tests/pkgconfig_static_fixture"
      -B "${prefix}/pkgconfig-static-${fixture_mode}"
      "-DCMAKE_PREFIX_PATH=${prefix}"
      -DCMAKE_DISABLE_FIND_PACKAGE_SDL3=TRUE
      -DCMAKE_DISABLE_FIND_PACKAGE_SDL3_image=TRUE
      -DCMAKE_DISABLE_FIND_PACKAGE_SDL3_ttf=TRUE)
    if(fixture_mode STREQUAL "existing")
      list(APPEND fixture_command -DUSE_EXISTING_SDL3=ON)
    endif()
    execute_process(COMMAND ${fixture_command}
      RESULT_VARIABLE fixture_configure_result)
    if(NOT fixture_configure_result EQUAL 0)
      message(FATAL_ERROR "pkg-config static fixture ${fixture_mode} configuration failed")
    endif()
    if(fixture_mode STREQUAL "default")
      execute_process(COMMAND "${CMAKE_COMMAND}" --build "${prefix}/pkgconfig-static-${fixture_mode}"
        RESULT_VARIABLE fixture_build_result)
      if(NOT fixture_build_result EQUAL 0)
        message(FATAL_ERROR "pkg-config static fixture compilation failed")
      endif()
    endif()
  endforeach()
endif()
