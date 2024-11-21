set(TUSB ${CMAKE_CURRENT_SOURCE_DIR}/tinyusb)

set(SOURCES
  main.cpp board.cpp board_msp.cpp drivers.cpp telemetry.cpp

  # dev
  device.cpp

  # drivers
  drivers/led.cpp
  drivers/serial.cpp
  drivers/beeper.cpp
  drivers/chiller.cpp
  drivers/brake.cpp
  drivers/motor.cpp
  drivers/speed.cpp
  drivers/usb.cpp

  drivers/drv_hx711.cpp
  drivers/drv_ina236.cpp
  drivers/drv_mpu6050.cpp
  drivers/drv_max6675.cpp

  # tinyUSB sources
  ${TUSB}/src/tusb.c
  ${TUSB}/src/common/tusb_fifo.c
  ${TUSB}/src/class/cdc/cdc_device.c
  ${TUSB}/src/device/usbd.c
  ${TUSB}/src/device/usbd_control.c
  ${TUSB}/src/portable/synopsys/dwc2/dcd_dwc2.c
  ${TUSB}/src/portable/synopsys/dwc2/dwc2_common.c
  usb_descriptors.c

  # system & other stuff
  sys/sysmem.c
  sys/syscalls.c
)

target_sources(stm32stand PRIVATE ${SOURCES})

target_include_directories(stm32stand PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(stm32stand PRIVATE ${TUSB}/hw)
target_include_directories(stm32stand PRIVATE ${TUSB}/src)
