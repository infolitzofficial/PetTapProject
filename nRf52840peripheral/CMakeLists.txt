cmake_minimum_required(VERSION 3.20.0)


set(BOARD_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(COMMON_PATH "C:/ncs/v2.4.2/modules/hal/nordic/nrfx/samples/common")
# include(${COMMON_PATH}/common.cmake)

# GET_DEVICE_CONFIG_FILES(${BOARD} boards)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nrfx_example)

target_sources(app PRIVATE src/main.c 
                           src/BLE/BleHandler.c
						   src/BLE/BleService.c
                           src/UartHandler/UartHandler.c
                           src/PacketHandler/PacketHandler.c
                           src/System/SystemHandler.c
                           "C:/ncs/v2.4.2/modules/hal/nordic/nrfx/samples/src/nrfx_saadc/common/saadc_examples_common.c")
target_include_directories(app PRIVATE src/BLE
                                       src/UartHandler
                                       src/PacketHandler
                                       src/System
                                       ${COMMON_PATH} "C:/ncs/v2.4.2/modules/hal/nordic/nrfx/samples/src/nrfx_saadc/common")
