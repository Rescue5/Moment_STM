cmake_minimum_required(VERSION 3.16)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE stm32_gcc.cmake) # FIXME
endif()

project(stm32stand C CXX ASM)
set(CMAKE_CXX_FLAGS -Wno-narrowing)

find_package(CMSIS COMPONENTS STM32F407VG REQUIRED)
find_package(HAL COMPONENTS STM32F4 CORTEX GPIO UART RCC SPI I2C DMA TIM REQUIRED)

add_executable(stm32stand)
add_subdirectory(src)

target_link_libraries(stm32stand
  HAL::STM32::F4
  HAL::STM32::F4::RCC
  HAL::STM32::F4::GPIO
  HAL::STM32::F4::UART
  HAL::STM32::F4::SPI
  HAL::STM32::F4::I2C
  HAL::STM32::F4::TIM
  HAL::STM32::F4::TIMEx
  HAL::STM32::F4::DMA
  HAL::STM32::F4::CORTEX
  CMSIS::STM32::F407VG
  STM32::NoSys
)

# fix for "warning: stm32stand.elf has a LOAD segment with RWX permissions"
target_link_options(stm32stand PRIVATE "LINKER:--no-warn-rwx-segments")

stm32_print_size_of_target(stm32stand)
