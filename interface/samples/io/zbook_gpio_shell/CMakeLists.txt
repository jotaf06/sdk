# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) Centro de Inovacao EDGE

cmake_minimum_required(VERSION 3.20.0)

if(NOT DEFINED BOARD)
    set(BOARD zbook@p2/rp2350b/m33)
endif()

# The IO channels come from the interface module's own "zbook-sdk" snippet
# (see sample.yaml's required_snippets), not a hand-wired overlay here.
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(zbook_gpio_shell)

target_sources(app PRIVATE src/main.c)
