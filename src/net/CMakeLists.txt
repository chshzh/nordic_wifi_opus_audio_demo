#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# Add network event management
target_sources(app PRIVATE
                     ${CMAKE_CURRENT_SOURCE_DIR}/net_event_mgmt.c)
# Add socket utilities
target_sources_ifdef(CONFIG_SOCKET_UTILS app PRIVATE
                     ${CMAKE_CURRENT_SOURCE_DIR}/socket_utils.c)
# Add Wi-Fi utilities
target_sources_ifdef(CONFIG_CONNECT_WITH_WIFI app PRIVATE
                     ${CMAKE_CURRENT_SOURCE_DIR}/wifi_utils.c)

target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
