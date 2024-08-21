# PicoMong: Pi Pico WiFi using Mongoose TCP/IP stack

cmake_minimum_required(VERSION 3.12)
include(pico_sdk_import.cmake)

# Console output on UART1
# add_definitions(-DPICO_DEFAULT_UART=1)

project(picomong C CXX ASM)

pico_sdk_init()

# Set to 0 for Pico-W CYW43439, 1 for Murata 1DX (CYW4343W)
# It may be necessary to reduce PIO_SPI_FREQ when using an external device
set (CHIP_4343W 0)

# Firmware file for CYW43439 or CYW4343W
if (${CHIP_4343W})
    message (STATUS "Building CYW4343W version")
    set (FW_FILE firmware/fw_4343w.c)
    add_compile_options(-DCHIP_4343W)
else ()
    message (STATUS "Building Pico-W CYW43439 version")
    set (FW_FILE firmware/fw_43439.c)
endif ()

# Tell CMake where to find the executable source file
add_executable(${PROJECT_NAME}
    picomong.c mg_wifi.c
    mongoose/mongoose.c mongoose/packed_fs.c mongoose/net.c 
    picowi/picowi_event.c picowi/picowi_init.c picowi/picowi_join.c
    picowi/picowi_pico.c picowi/picowi_pio.c picowi/picowi_wifi.c
    picowi/picowi_ioctl.c ${FW_FILE} )

# Enable compiler warnings
add_compile_options(-Wall)

# Create map/bin/hex/uf2 files
pico_add_extra_outputs(${PROJECT_NAME})

# Create PIO header file
pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/picowi/picowi_pio.pio)

# Link to pico_stdlib (gpio, time, etc. functions)
target_link_libraries(${PROJECT_NAME} 
    pico_stdlib hardware_spi pico_rand hardware_pio 
    hardware_dma hardware_i2c hardware_pwm )

# Mongoose build flags
add_definitions(-DMG_ENABLE_TCPIP=1)
add_definitions(-DMG_ENABLE_PACKED_FS=1)
add_definitions(-DMG_ENABLE_MBEDTLS=0) # TODO(cpq): enable
add_definitions(-DMG_ENABLE_CUSTOM_RANDOM=0)
add_definitions(-DMG_ENABLE_FILE=0)
add_definitions(-DMG_ARCH=MG_ARCH_RP2040)
add_definitions(-DHTTP_URL="0.0.0.0:80")
add_definitions(-DHTTPS_URL="0.0.0.0:443")

# Enable uart output, disable usb output
pico_enable_stdio_usb(${PROJECT_NAME} 0)
pico_enable_stdio_uart(${PROJECT_NAME} 1)

# EOF