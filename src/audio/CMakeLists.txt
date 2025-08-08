#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause


FILE(GLOB audio_sources 
        ${CMAKE_CURRENT_SOURCE_DIR}/*.c
        )

target_sources(app PRIVATE
        ${audio_sources}
        )

target_include_directories(app PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        )


