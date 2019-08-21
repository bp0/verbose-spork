project(bsysinfo C)
cmake_minimum_required(VERSION 2.6)

if(${CMAKE_BUILD_TYPE} MATCHES [Dd]ebug)
        message("sysobj debug build")
        add_definitions(-DSYSOB_DEBUG_BUILD=1)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address")
endif()

add_definitions(-DSYSOB_SRC_ROOT="${CMAKE_CURRENT_SOURCE_DIR}")

include(FindPkgConfig)
pkg_check_modules(SYSOB_GLIB REQUIRED glib-2.0>=2.10)
pkg_check_modules(SYSOB_GTK3 gtk+-3.0>=3.12)

add_definitions("-std=gnu99")
message("sysobj source dir: ${CMAKE_CURRENT_SOURCE_DIR}")
message("sysobj binary dir: ${CMAKE_CURRENT_BINARY_DIR}")

include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include/sysobj
        ${CMAKE_CURRENT_SOURCE_DIR}/gui
        ${CMAKE_CURRENT_SOURCE_DIR}/uber-graph
        ${SYSOB_GLIB_INCLUDE_DIRS}
        ${SYSOB_GTK3_INCLUDE_DIRS}
        ${CMAKE_CURRENT_BINARY_DIR}
)
link_directories(
        ${SYSOB_GLIB_LIBRARY_DIRS}
        ${SYSOB_GTK3_LIBRARY_DIRS}
)

add_library(sysobj STATIC
        sysobj/src/sysobj.c
        sysobj/src/sysobj_virt.c
        sysobj/src/sysobj_filter.c
        sysobj/src/sysobj_foreach.c
        sysobj/src/util_sysobj.c
        sysobj/src/auto_free.c
        sysobj/src/appf.c
        sysobj/src/gg_file.c
        sysobj/src/gg_slist.c
        sysobj/src/strstr_word.c
        sysobj/src/nice_name.c
        sysobj/src/format_funcs.c
        sysobj/src/pin.c
        sysobj/src/classes.c
        sysobj/src/cpubits.c

        sysobj/src/class_cpu.c
        sysobj/src/class_cpufreq.c
        sysobj/src/class_cpucache.c
        sysobj/src/class_cputopo.c
        sysobj/src/class_cpuidle.c
        sysobj/src/class_any_utf8.c
        sysobj/src/class_any_bus.c
        sysobj/src/class_extra.c

# still mixed
        sysobj/src/class_pci.c
        sysobj/src/class_usb.c

# class only
        sysobj/src/class_sysobj.c
        sysobj/src/class_lookup.c
        sysobj/src/class_dt.c
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
        sysobj/src/class_hwmon.c
        sysobj/src/class_devfreq.c
        sysobj/src/class_backlight.c
        sysobj/src/class_i2c.c
        sysobj/src/class_block.c
        sysobj/src/class_scsi.c
        sysobj/src/class_clocksource.c
        sysobj/src/class_mmc.c
        sysobj/src/class_media.c
        sysobj/src/class_intel_pstate.c
        sysobj/src/class_nvme.c
        sysobj/src/class_storage.c
        sysobj/src/class_aer.c
        sysobj/src/class_power_supply.c

# gen only
        sysobj/src/gen_arm_ids.c
        sysobj/src/gen_sdio_ids.c
        sysobj/src/gen_sdcard_ids.c
        sysobj/src/gen_dt.c
        sysobj/src/gen_edid_ids.c
        sysobj/src/gen_ieee_oui_ids.c
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
        sysobj/src/gen_storage.c

        sysobj/src/arm_data.c
        sysobj/src/x86_data.c
        sysobj/src/riscv_data.c

        sysobj/src/util_edid.c
        sysobj/src/util_dt.c
        sysobj/src/util_pci.c
        sysobj/src/util_usb.c
        sysobj/src/util_ids.c
        sysobj/src/vendor.c
)
target_link_libraries(sysobj
        ${SYSOB_GLIB_LIBRARIES}
        m
)
set_target_properties(sysobj PROPERTIES COMPILE_FLAGS "-O3 -Werror=implicit-function-declaration")

#set_target_properties(sysobj PROPERTIES COMPILE_FLAGS "-Wall -Wextra -Werror")

set_source_files_properties(
        sysobj/src/x86_data.c
        PROPERTIES
        COMPILE_FLAGS "-Wno-unused-function"
)

add_executable(bsysinfo
        src/bsysinfo.c
)
target_link_libraries(bsysinfo
        ${SYSOB_GLIB_LIBRARIES}
        sysobj
)

add_executable(test_vendor src/test_vendor.c)
target_link_libraries(test_vendor ${SYSOB_GLIB_LIBRARIES} sysobj)
add_executable(test_dt_compat src/test_dt_compat.c)
target_link_libraries(test_dt_compat ${SYSOB_GLIB_LIBRARIES} sysobj)
add_executable(test_nice_name src/test_nice_name.c)
target_link_libraries(test_nice_name ${SYSOB_GLIB_LIBRARIES} sysobj)
add_executable(test_edid src/test_edid.c)
target_link_libraries(test_edid ${SYSOB_GLIB_LIBRARIES} sysobj)

if(SYSOB_GTK3_FOUND)
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
add_library(sysobj_widgets STATIC
        gui/bp_sysobj_search.c
        gui/bp_sysobj_browser.c
        gui/bp_sysobj_view.c
        gui/bp_pin_inspect.c
        gui/uri_handler.c
)
target_link_libraries(sysobj_widgets
        ${SYSOB_GLIB_LIBRARIES}
        ${SYSOB_GTK3_LIBRARIES}
        sysobj
)
set_target_properties(sysobj_widgets PROPERTIES COMPILE_FLAGS "-Wno-deprecated-declarations")
add_executable(bsysinfo-gtk
        gui/bsysinfo-gtk.c
)
set_target_properties(bsysinfo-gtk PROPERTIES COMPILE_FLAGS "-Wno-deprecated-declarations")
target_link_libraries(bsysinfo-gtk
        ${SYSOB_GLIB_LIBRARIES}
        ${SYSOB_GTK3_LIBRARIES}
        sysobj
        sysobj_widgets
        m
)
else()
message(STATUS "GTK3 libs not found, skipping GUI")
endif()

configure_file(sysobj_config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/sysobj_config.h @ONLY)
configure_file("data/pci.ids" ${CMAKE_CURRENT_BINARY_DIR}/ COPYONLY)
configure_file("data/usb.ids" ${CMAKE_CURRENT_BINARY_DIR}/ COPYONLY)
configure_file("data/dt.ids" ${CMAKE_CURRENT_BINARY_DIR}/ COPYONLY)
configure_file("data/vendor.ids" ${CMAKE_CURRENT_BINARY_DIR}/ COPYONLY)
configure_file("data/arm.ids" ${CMAKE_CURRENT_BINARY_DIR}/ COPYONLY)
configure_file("data/sdio.ids" ${CMAKE_CURRENT_BINARY_DIR}/ COPYONLY)
configure_file("data/sdcard.ids" ${CMAKE_CURRENT_BINARY_DIR}/ COPYONLY)
configure_file("data/edid.ids" ${CMAKE_CURRENT_BINARY_DIR}/ COPYONLY)
configure_file("data/ieee_oui.ids" ${CMAKE_CURRENT_BINARY_DIR}/ COPYONLY)
configure_file("data/x86.funfacts" ${CMAKE_CURRENT_BINARY_DIR}/ COPYONLY)
