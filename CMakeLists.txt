cmake_minimum_required(VERSION 3.20)

project(stm32_workspace)

# ------------------------------------------------------------
# CONFIG
# ------------------------------------------------------------

set(STM32_DIR ${CMAKE_SOURCE_DIR}/stm32)
set(APP_DIR ${CMAKE_SOURCE_DIR}/app)
set(STM32_BUILD_DIR ${STM32_DIR}/build)

# ------------------------------------------------------------
# BUILD (CubeMX project as external build)
# ------------------------------------------------------------

add_custom_target(firmware_configure
    COMMAND ${CMAKE_COMMAND}
        -S ${STM32_DIR}
        -B ${STM32_BUILD_DIR}
        -DAPP_DIR=${APP_DIR}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_custom_target(firmware_build
    COMMAND ${CMAKE_COMMAND} --build ${STM32_BUILD_DIR}
    DEPENDS firmware_configure
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_custom_target(firmware
    DEPENDS firmware_build
)

# ------------------------------------------------------------
# OPTIONAL: CLEAN
# ------------------------------------------------------------

add_custom_target(firmware_clean
    COMMAND ${CMAKE_COMMAND} -E rm -rf ${STM32_BUILD_DIR}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)