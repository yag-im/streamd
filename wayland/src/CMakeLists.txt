cmake_minimum_required(VERSION 3.13)

project(streamd LANGUAGES C)

set(CMAKE_VERBOSE_MAKEFILE "ON")
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0 gstreamer-base-1.0)
pkg_check_modules(GIO REQUIRED gio-2.0)
pkg_check_modules(WAYLAND REQUIRED wayland-client)
pkg_check_modules(XKB REQUIRED xkbcommon)

include(CheckFunctionExists)
check_function_exists(memfd_create HAVE_MEMFD_CREATE)
if (HAVE_MEMFD_CREATE)
	set(HAVE_MEMFD TRUE)
else()
	check_function_exists(SYS_memfd_create HAVE_MEMFD_SYS)
	if (HAVE_MEMFD_SYS)
		set(HAVE_MEMFD TRUE)
	endif()
endif()
configure_file(config.h.in config.h @ONLY)

include_directories(${CMAKE_SOURCE_DIR}/src/navi ${CMAKE_SOURCE_DIR}/wayland/src/navi ${CMAKE_SOURCE_DIR}/build/wayland/src ${CMAKE_SOURCE_DIR}/build/src)

set(
    SOURCE_FILES
    main.c
    log.c
    libobs/util/base.c
    libobs/util/bmem.c
    libobs/util/dstr.c
    navi/intset.c
    navi/keyboard.c
    navi/navi.c
    navi/pointer.c
    navi/shm.c
    navi/virtual-keyboard-unstable-v1-client-protocol.c
    navi/wlr-virtual-pointer-unstable-v1-client-protocol.c
    portal/portal.c
    portal/screencast-portal.c
)

add_executable(streamd ${SOURCE_FILES})

if(VIDEO_ENC)
    target_compile_definitions(streamd PRIVATE "VIDEO_ENC=\"${VIDEO_ENC}\"")
endif()

if(LOG_LEVEL)
    target_compile_definitions(streamd PRIVATE "LOG_LEVEL=${LOG_LEVEL}")
endif()

target_link_libraries(streamd ${GSTREAMER_LIBRARIES} ${GIO_LIBRARIES} ${WAYLAND_LIBRARIES} ${XKB_LIBRARIES} gstvideo-1.0.so )
target_include_directories(streamd PUBLIC ${GSTREAMER_INCLUDE_DIRS} ${GIO_LIBRARY_DIRS} libobs)
target_compile_options(streamd PUBLIC ${GSTREAMER_CFLAGS_OTHER} ${GIO_CFLAGS_OTHER})
