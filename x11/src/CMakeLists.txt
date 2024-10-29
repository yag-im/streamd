cmake_minimum_required(VERSION 3.13)

project(streamd LANGUAGES C)

set(CMAKE_VERBOSE_MAKEFILE "ON")
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0 gstreamer-base-1.0)

set(
    SOURCE_FILES
    main.c
    navi.c
)

add_executable(streamd ${SOURCE_FILES})

if(VIDEO_ENC)
    target_compile_definitions(streamd PRIVATE "VIDEO_ENC=\"${VIDEO_ENC}\"")
endif()

target_link_libraries(streamd ${GSTREAMER_LIBRARIES} gstvideo-1.0.so Xtst X11)
target_include_directories(streamd PUBLIC ${GSTREAMER_INCLUDE_DIRS} /include/gstreamer-1.0)
target_compile_options(streamd PUBLIC ${GSTREAMER_CFLAGS_OTHER})
