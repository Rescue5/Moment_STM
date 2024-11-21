# stm32stand

## Настройка сборочного окружения

Для работы потребуется `git`, `make`, `cmake`.

С помощью `git` нужно предварительно подтянуть сумбодуль
`src/tinyusb`.

### Тулчейн

Скачать и установить ПО для кросс-компиляции:

https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads

Файл: arm-gnu-toolchain-13.3.rel1-x86_64-arm-none-eabi.tar.xz (или новее)

### stlink (предпочтительно)

~~~
$ sudo apt install stlink-tools
~~~

### jlink (опционально, альтернатива stlink)

Скачать и установить ПО для J-Link:

https://www.segger.com/downloads/jlink/

Файл: JLink_Linux_V798f_x86_64.deb (или новее)

Добавить правила `udev` для программатора создав файл
`/etc/udev/rules.d/50-jlink.rules` содержащий:

~~~
ACTION!="add", SUBSYSTEM!="usb_device", GOTO="jlink_rules_end"

# udevadm test --action=add $(udevadm info -q path -n /dev/bus/usb/xxx/yyy)
ATTR{idProduct}=="0101", ATTR{idVendor}=="1366", MODE="666"

LABEL="jlink_rules_end"
~~~

После создания файла правил активировать его выполнив команду:

~~~
# udevadm control --reload-rules
~~~

## Установка внешних зависимостей

В процессе сборки используются следующие зависимости:

1) STMCubeF4

https://github.com/STMicroelectronics/STM32CubeF4.git

Клонировать проект командой `git clone ...`

2) stm32-cmake

https://github.com/ObKo/stm32-cmake

Клонировать проект командой `git clone ...`

## Сборка

Для сборки используется `make` + `cmake`. Корневой `Makefile`
потребует до-настройки на соответствующие пути через переменные
окружения (stm32cube, stm32cmake, stm32tools). Далее выполнить:

~~~
$ make <debug|release>
~~~

## Прошивка

~~~
$ make flash
~~~
