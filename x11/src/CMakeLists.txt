cmake_minimum_required(VERSION 3.13)

project(streamd LANGUAGES C)

set(CMAKE_VERBOSE_MAKEFILE "ON")
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0 gstreamer-base-1.0)

set(
    SOURCE_FILES
    log.c
    main.c
    navi.c
    utils.c
)

add_executable(streamd ${SOURCE_FILES})

if(VIDEO_ENC)
    target_compile_definitions(streamd PRIVATE "VIDEO_ENC=\"${VIDEO_ENC}\"")
endif()

if(LOG_LEVEL)
    target_compile_definitions(streamd PRIVATE "LOG_LEVEL=${LOG_LEVEL}")
endif()

target_link_libraries(streamd ${CMAKE_THREAD_LIBS_INIT} ${GSTREAMER_LIBRARIES} gstvideo-1.0.so Xtst X11)
target_include_directories(streamd PUBLIC ${GSTREAMER_INCLUDE_DIRS})
target_compile_options(streamd PUBLIC ${GSTREAMER_CFLAGS_OTHER})
