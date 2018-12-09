project(bsysinfo C)
cmake_minimum_required(VERSION 2.6)

if(${CMAKE_BUILD_TYPE} MATCHES [Dd]ebug)
        message("Debug build")
        add_definitions(-DDEBUG_BUILD=1)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address")
endif()

add_definitions(-DSRC_ROOT="${CMAKE_SOURCE_DIR}")

include(FindPkgConfig)
pkg_check_modules(GLIB REQUIRED glib-2.0>=2.10)
pkg_check_modules(GTK gtk+-3.0>=3.12)

include_directories(
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/include/sysobj
        ${CMAKE_SOURCE_DIR}/gui
        ${CMAKE_SOURCE_DIR}/uber-graph
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
        sysobj/src/sysobj.c
        sysobj/src/sysobj_filter.c
        sysobj/src/sysobj_foreach.c
        sysobj/src/util_sysobj.c
        sysobj/src/gg_file.c
        sysobj/src/pin.c
        sysobj/src/classes.c
        sysobj/src/cpubits.c

        sysobj/src/class_cpu.c
        sysobj/src/class_cpufreq.c
        sysobj/src/class_cpucache.c
        sysobj/src/class_cputopo.c
        sysobj/src/class_any_utf8.c

# still mixed
        sysobj/src/class_dt.c
        sysobj/src/class_pci.c
        sysobj/src/class_usb.c

# class only
        sysobj/src/class_sysfs.c
        sysobj/src/class_os_release.c
        sysobj/src/class_mobo.c
        sysobj/src/class_rpi.c
        sysobj/src/class_cpuinfo.c
        sysobj/src/class_meminfo.c
        sysobj/src/class_procs.c
        sysobj/src/class_dmi_id.c
        sysobj/src/class_uptime.c
        sysobj/src/class_proc_alts.c
        sysobj/src/class_gpu.c
# gen only
        sysobj/src/gen_sysobj.c
        sysobj/src/gen_usb_ids.c
        sysobj/src/gen_pci_ids.c
        sysobj/src/gen_dt_ids.c
        sysobj/src/gen_os_release.c
        sysobj/src/gen_dmidecode.c
        sysobj/src/gen_mobo.c
        sysobj/src/gen_rpi.c
        sysobj/src/gen_cpuinfo.c
        sysobj/src/gen_meminfo.c
        sysobj/src/gen_procs.c
        sysobj/src/gen_gpu.c

        sysobj/src/arm_data.c
        sysobj/src/x86_data.c
#src/riscv_data.c

        sysobj/src/util_pci.c
        sysobj/src/util_usb.c
)
target_link_libraries(sysobj
        gettext
)
#set_target_properties(sysobj PROPERTIES COMPILE_FLAGS "-Wall -Wextra -Werror")

set_source_files_properties(
        src/x86_data.c
        PROPERTIES
        COMPILE_FLAGS "-Wno-unused-function"
)

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
if(0)
add_library(uber-graph STATIC
        uber-graph/g-ring.c
        uber-graph/uber-frame-source.c
        uber-graph/uber-graph.c
        uber-graph/uber-heat-map.c
        uber-graph/uber-label.c
        uber-graph/uber-line-graph.c
        uber-graph/uber-range.c
        uber-graph/uber-scale.c
        uber-graph/uber-scatter.c
        uber-graph/uber-timeout-interval.c
        uber-graph/uber-window.c
)
set_target_properties(uber-graph PROPERTIES COMPILE_FLAGS "-Wno-deprecated-declarations")
endif()
add_executable(bsysinfo-gtk
        gui/bsysinfo-gtk.c
        gui/bp_sysobj_search.c
        gui/bp_sysobj_browser.c
        gui/bp_sysobj_view.c
        gui/bp_pin_inspect.c
        gui/uri_handler.c
)
set_target_properties(bsysinfo-gtk PROPERTIES COMPILE_FLAGS "-Wno-deprecated-declarations")
target_link_libraries(bsysinfo-gtk
        ${GLIB_LIBRARIES}
        ${GTK_LIBRARIES}
        gettext
        sysobj
        m
)
else()
message(STATUS "GTK3 libs not found, skipping GUI")
endif()

configure_file(config.h.cmake ${CMAKE_BINARY_DIR}/config.h @ONLY)
configure_file("data/pci.ids" ${CMAKE_BINARY_DIR}/ COPYONLY)
configure_file("data/usb.ids" ${CMAKE_BINARY_DIR}/ COPYONLY)
configure_file("data/dt.ids" ${CMAKE_BINARY_DIR}/ COPYONLY)
