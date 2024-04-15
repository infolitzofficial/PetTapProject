#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

set(COMMON_PATH "C:/ncs/v2.4.2/modules/hal/nordic/nrfx/samples/common")
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(gnss_sample)

zephyr_library_sources(src/main.c
                    src/WiFi/WiFiHandler.c
                    src/System/SystemHandler.c
                    src/BLE/BleHandler.c
                    src/PacketHandler/PacketHandler.c
                    src/NVS/NvsHandler.c
                    src/Temp_MCP/TempAdcHandler.c
                    "C:/ncs/v2.4.2/modules/hal/nordic/nrfx/samples/src/nrfx_saadc/common/saadc_examples_common.c")
target_include_directories(app PRIVATE ${COMMON_PATH} "C:/ncs/v2.4.2/modules/hal/nordic/nrfx/samples/src/nrfx_saadc/common")

zephyr_library_sources_ifdef(CONFIG_GNSS_SAMPLE_ASSISTANCE_NRF_CLOUD src/assistance.c)
zephyr_library_sources_ifdef(CONFIG_GNSS_SAMPLE_ASSISTANCE_SUPL src/assistance_supl.c)
zephyr_library_sources_ifdef(CONFIG_GNSS_SAMPLE_ASSISTANCE_MINIMAL src/assistance_minimal.c)
zephyr_library_sources_ifdef(CONFIG_GNSS_SAMPLE_ASSISTANCE_MINIMAL src/mcc_location_table.c)
