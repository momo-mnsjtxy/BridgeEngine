if(NOT DEFINED BUILD_DIR OR NOT DEFINED SOURCE_DIR OR NOT DEFINED C_COMPILER)
  message(FATAL_ERROR "BUILD_DIR, SOURCE_DIR, and C_COMPILER are required")
endif()

set(prefix "${BUILD_DIR}/install-smoke")
file(REMOVE_RECURSE "${prefix}")
execute_process(COMMAND "${CMAKE_COMMAND}" --install "${BUILD_DIR}" --prefix "${prefix}"
  RESULT_VARIABLE install_result)
if(NOT install_result EQUAL 0)
  message(FATAL_ERROR "installation failed")
endif()
execute_process(COMMAND "${C_COMPILER}" -std=c11 "-I${prefix}/include"
  "${SOURCE_DIR}/tests/master_aggregate_bapi_compat_test.c" "-L${prefix}/lib" -lbridgeengine
  "-Wl,-rpath,${prefix}/lib" -o "${prefix}/aggregate-smoke"
  RESULT_VARIABLE compile_result)
if(NOT compile_result EQUAL 0)
  message(FATAL_ERROR "external aggregate compilation failed")
endif()
execute_process(COMMAND "${prefix}/aggregate-smoke" RESULT_VARIABLE run_result)
if(NOT run_result EQUAL 0)
  message(FATAL_ERROR "external aggregate program failed")
endif()
execute_process(COMMAND "${CMAKE_COMMAND}" -S "${SOURCE_DIR}/tests/package_consumer"
  -B "${prefix}/package-consumer" "-DCMAKE_PREFIX_PATH=${prefix}"
  RESULT_VARIABLE package_configure_result)
if(NOT package_configure_result EQUAL 0)
  message(FATAL_ERROR "find_package consumer configuration failed")
endif()
execute_process(COMMAND "${CMAKE_COMMAND}" --build "${prefix}/package-consumer"
  RESULT_VARIABLE package_build_result)
if(NOT package_build_result EQUAL 0)
  message(FATAL_ERROR "find_package consumer build failed")
endif()
