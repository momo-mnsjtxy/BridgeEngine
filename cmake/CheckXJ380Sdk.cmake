if(NOT DEFINED XJ380_SDK)
	message(FATAL_ERROR "XJ380_SDK is not set")
endif()

if(NOT IS_DIRECTORY "${XJ380_SDK}/include")
	message(FATAL_ERROR "XJ380 SDK headers not found: ${XJ380_SDK}/include")
endif()

if(NOT IS_DIRECTORY "${XJ380_SDK}/obj-gui")
	message(FATAL_ERROR "XJ380 SDK GUI objects not found: ${XJ380_SDK}/obj-gui")
endif()

file(GLOB_RECURSE xj380_gui_objects
	"${XJ380_SDK}/obj-gui/*.o"
)

if(NOT xj380_gui_objects)
	message(FATAL_ERROR "XJ380 SDK GUI object files are missing under ${XJ380_SDK}/obj-gui")
endif()

if(NOT EXISTS "${XJ380_SDK}/obj-gui/liballoc-x86_64.a")
	message(FATAL_ERROR "XJ380 SDK archive not found: ${XJ380_SDK}/obj-gui/liballoc-x86_64.a")
endif()
