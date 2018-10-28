project(bsysinfo C)
cmake_minimum_required(VERSION 2.6)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror")

if(${CMAKE_BUILD_TYPE} MATCHES [Dd]ebug)
        message("Debug build")
        add_definitions(-DDEBUG_BUILD=1)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address")
endif()

include(FindPkgConfig)
pkg_check_modules(GLIB REQUIRED glib-2.0>=2.10)

include_directories(
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/include
        ${GLIB_INCLUDE_DIRS}
        ${CMAKE_BINARY_DIR}
)
link_directories(
        ${GLIB_LIBRARY_DIRS}
)

add_executable(bsysinfo
        src/bsysinfo.c
        src/gettext.c
        src/sysobj.c
        src/pin.c
        src/classes.c
        src/class_dmi_id.c
        src/class_cpu.c
        src/class_cpufreq.c
        src/class_cpucache.c
)
target_link_libraries(bsysinfo
        ${GLIB_LIBRARIES}
)

configure_file(config.h.cmake ${CMAKE_BINARY_DIR}/config.h @ONLY)
