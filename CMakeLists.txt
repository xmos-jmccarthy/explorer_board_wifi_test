cmake_minimum_required(VERSION 3.20)

# Disable in-source build.
if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
    message(FATAL_ERROR "In-source build is not allowed! Please specify a build folder.\n\tex:cmake -B build")
endif()

## Specify configuration
set(USE_WIFI_MANAGER TRUE)
set(USE_DHCPD TRUE)
set(USE_FATFS TRUE)

## Import XMOS configurations
include("$ENV{XCORE_SDK_PATH}/tools/cmake_utils/xmos_platform.cmake")

## Define project
project(explorer_wifi_test)
set(PROJECT_VERSION 0.6.0)

## Enable languages for project
enable_language(CXX C ASM)

## Set XMOS application variables
set(APP_COMPILER_FLAGS
    "-Os"
    "-g"
    "-report"
    "-fxscope"
    "-lquadspi"
    "-mcmodel=large"
    "-Wno-xcore-fptrgroup"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/config.xscope"
    "${CMAKE_CURRENT_SOURCE_DIR}/${BOARD}.xn"
)

set(APP_SOURCES
    "src/main.c"
)

set(APP_INCLUDES
    "src"
)

add_compile_definitions(
    DEBUG_PRINT_ENABLE=1

    PLATFORM_USES_TILE_0=1
)

if(${VERBOSE})
    add_compile_definitions(SL_WFX_DEBUG_MASK=\(SL_WFX_DEBUG_ERROR|SL_WFX_DEBUG_INIT\))
else()
    add_compile_definitions(SL_WFX_DEBUG_MASK=\(SL_WFX_DEBUG_ERROR\))
endif()

set(TARGET_NAME "${PROJECT_NAME}.xe")

add_executable(${TARGET_NAME})

target_sources(${TARGET_NAME} PRIVATE ${APP_SOURCES} ${XMOS_RTOS_PLATFORM_WITH_NETWORKING_SOURCES})
target_include_directories(${TARGET_NAME} PRIVATE ${APP_INCLUDES} ${XMOS_RTOS_PLATFORM_WITH_NETWORKING_INCLUDES})

target_compile_options(${TARGET_NAME} PRIVATE ${APP_COMPILER_FLAGS})
target_link_options(${TARGET_NAME} PRIVATE ${APP_COMPILER_FLAGS})
