#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

target_sources_ifdef(CONFIG_HEAPS_MONITOR app PRIVATE
                     ${CMAKE_CURRENT_SOURCE_DIR}/heaps_monitor.c)
