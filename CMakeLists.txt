# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

set(FREERTOS_KERNEL_PATH "/home/emyle-silva/FreeRTOS-Kernel")
include(${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/FreeRTOS_Kernel_import.cmake)

project(Tarefa5_MonitoramentoEnchentesFreeRTOS C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

include_directories(${CMAKE_SOURCE_DIR}/lib)

# Add executable. Default name is the project name, version 0.1

add_executable(Tarefa5_MonitoramentoEnchentesFreeRTOS Tarefa5_MonitoramentoEnchentesFreeRTOS.c lib/ssd1306.c)

pico_set_program_name(Tarefa5_MonitoramentoEnchentesFreeRTOS "Tarefa5_MonitoramentoEnchentesFreeRTOS")
pico_set_program_version(Tarefa5_MonitoramentoEnchentesFreeRTOS "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(Tarefa5_MonitoramentoEnchentesFreeRTOS 1)
pico_enable_stdio_usb(Tarefa5_MonitoramentoEnchentesFreeRTOS 1)

pico_generate_pio_header(Tarefa5_MonitoramentoEnchentesFreeRTOS ${CMAKE_CURRENT_LIST_DIR}/pio_matrix.pio)

# Add the standard library to the build
target_link_libraries(Tarefa5_MonitoramentoEnchentesFreeRTOS
        pico_stdlib 
        hardware_gpio
        hardware_i2c
        hardware_adc
        hardware_clocks
        hardware_uart
        hardware_pio
        hardware_pwm
        pico_time
        FreeRTOS-Kernel 
        FreeRTOS-Kernel-Heap4)

# Add the standard include files to the build
target_include_directories(Tarefa5_MonitoramentoEnchentesFreeRTOS PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)

pico_add_extra_outputs(Tarefa5_MonitoramentoEnchentesFreeRTOS)

