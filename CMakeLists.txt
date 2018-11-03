project(bsysinfo C)
cmake_minimum_required(VERSION 2.6)

#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror")

if(${CMAKE_BUILD_TYPE} MATCHES [Dd]ebug)
        message("Debug build")
        add_definitions(-DDEBUG_BUILD=1)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address")
endif()

include(FindPkgConfig)
pkg_check_modules(GLIB REQUIRED glib-2.0>=2.10)
pkg_check_modules(GTK gtk+-3.0>=3.12)

include_directories(
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/gui
        ${GLIB_INCLUDE_DIRS}
        ${GTK_INCLUDE_DIRS}
        ${CMAKE_BINARY_DIR}
)
link_directories(
        ${GLIB_LIBRARY_DIRS}
        ${GTK_LIBRARY_DIRS}
)

add_library(gettext STATIC
        src/gettext.c
)

add_library(sysobj STATIC
        src/sysobj.c
        src/pin.c
        src/classes.c
        src/class_dmi_id.c
        src/class_dt.c
        src/class_cpu.c
        src/class_cpufreq.c
        src/class_cpucache.c
        src/class_cputopo.c
        src/class_any_utf8.c
        src/vo_computer.c
)
target_link_libraries(sysobj
        gettext
)
set_target_properties(sysobj PROPERTIES COMPILE_FLAGS "-Wall -Wextra -Werror")

add_executable(bsysinfo
        src/bsysinfo.c
)
target_link_libraries(bsysinfo
        ${GLIB_LIBRARIES}
        gettext
        sysobj
)

if(GTK_FOUND)
add_definitions(-DGTK_DISABLE_SINGLE_INCLUDES)
add_executable(bsysinfo-gtk
        gui/bsysinfo-gtk.c
)
target_link_libraries(bsysinfo-gtk
        ${GLIB_LIBRARIES}
        ${GTK_LIBRARIES}
        gettext
        sysobj
)
else()
message(STATUS "GTK3 libs not found, skipping GUI")
endif()

configure_file(config.h.cmake ${CMAKE_BINARY_DIR}/config.h @ONLY)
