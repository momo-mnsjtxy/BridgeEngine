#[=======================================================================[.rst:
FindFFmpeg
----------

Find FFmpeg multimedia libraries: libavcodec, libavformat, libavutil,
libswscale, libswresample.

Imported targets
^^^^^^^^^^^^^^^^

This module defines the following :prop_tgt:`IMPORTED` targets:

``FFmpeg::avcodec``
``FFmpeg::avformat``
``FFmpeg::avutil``
``FFmpeg::swscale``
``FFmpeg::swresample``

Result variables
^^^^^^^^^^^^^^^^

``FFmpeg_FOUND``
  True if all FFmpeg components were found.
``FFmpeg_INCLUDE_DIRS``
  Include directories for FFmpeg.
``FFmpeg_LIBRARIES``
  All FFmpeg libraries as a list.
#]=======================================================================]

include(FindPackageHandleStandardArgs)

foreach(COMPONENT avcodec avformat avutil swscale swresample)
    string(TOUPPER ${COMPONENT} COMPONENT_UPPER)

    find_path(FFmpeg_${COMPONENT_UPPER}_INCLUDE_DIR
        NAMES lib${COMPONENT}/${COMPONENT}.h
        PATHS
            /usr/include
            /usr/local/include
            /opt/homebrew/include
            $ENV{VCPKG_ROOT}/packages/*/include
            $ENV{VCPKG_ROOT}/installed/*/include
        PATH_SUFFIXES ffmpeg
    )

    find_library(FFmpeg_${COMPONENT_UPPER}_LIBRARY
        NAMES ${COMPONENT}
        PATHS
            /usr/lib
            /usr/local/lib
            /opt/homebrew/lib
            $ENV{VCPKG_ROOT}/packages/*/lib
            $ENV{VCPKG_ROOT}/installed/*/lib
    )

    if(FFmpeg_${COMPONENT_UPPER}_LIBRARY AND FFmpeg_${COMPONENT_UPPER}_INCLUDE_DIR)
        set(FFmpeg_${COMPONENT_UPPER}_FOUND TRUE)
        if(NOT TARGET FFmpeg::${COMPONENT})
            add_library(FFmpeg::${COMPONENT} UNKNOWN IMPORTED)
            set_target_properties(FFmpeg::${COMPONENT} PROPERTIES
                IMPORTED_LOCATION "${FFmpeg_${COMPONENT_UPPER}_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${FFmpeg_${COMPONENT_UPPER}_INCLUDE_DIR}"
            )
        endif()
    endif()

    list(APPEND FFmpeg_INCLUDE_DIRS ${FFmpeg_${COMPONENT_UPPER}_INCLUDE_DIR})
    list(APPEND FFmpeg_LIBRARIES ${FFmpeg_${COMPONENT_UPPER}_LIBRARY})
endforeach()

find_package_handle_standard_args(FFmpeg
    REQUIRED_VARS
        FFmpeg_AVCODEC_LIBRARY FFmpeg_AVCODEC_INCLUDE_DIR
        FFmpeg_AVFORMAT_LIBRARY FFmpeg_AVFORMAT_INCLUDE_DIR
        FFmpeg_AVUTIL_LIBRARY FFmpeg_AVUTIL_INCLUDE_DIR
        FFmpeg_SWSCALE_LIBRARY FFmpeg_SWSCALE_INCLUDE_DIR
        FFmpeg_SWRESAMPLE_LIBRARY FFmpeg_SWRESAMPLE_INCLUDE_DIR
)

mark_as_advanced(
    FFmpeg_AVCODEC_INCLUDE_DIR FFmpeg_AVCODEC_LIBRARY
    FFmpeg_AVFORMAT_INCLUDE_DIR FFmpeg_AVFORMAT_LIBRARY
    FFmpeg_AVUTIL_INCLUDE_DIR FFmpeg_AVUTIL_LIBRARY
    FFmpeg_SWSCALE_INCLUDE_DIR FFmpeg_SWSCALE_LIBRARY
    FFmpeg_SWRESAMPLE_INCLUDE_DIR FFmpeg_SWRESAMPLE_LIBRARY
)
