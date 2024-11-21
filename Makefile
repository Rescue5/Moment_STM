stm32cube  ?= /Users/macbook/Documents/работа/STM/1645de931ef08a0f5f066d94b725e7a7b9e74735/STM32CubeF4
stm32cmake ?= $(CURDIR)/stm32-cmake
stm32tools ?= $(CURDIR)/arm-gnu-toolchain-13.3.rel1-darwin-arm64-arm-none-eabi

# custom cmake parameners
CMAKE_ARGS += -DCMAKE_MODULE_PATH=$(stm32cmake)/cmake
CMAKE_ARGS += -DCMAKE_TOOLCHAIN_FILE=$(stm32cmake)/cmake/stm32_gcc.cmake

# custom stm32 parameters
CMAKE_ARGS += -DSTM32_CUBE_F4_PATH=$(stm32cube)
CMAKE_ARGS += -DSTM32_TOOLCHAIN_PATH=$(stm32tools)
CMAKE_ARGS += -DSTM32_TARGET_TRIPLET=arm-none-eabi

################################################################################

debug: do-debug
do-debug: CMAKE_ARGS += -DCMAKE_BUILD_TYPE=Debug
do-debug: do-build

release: do-release
do-release: CMAKE_ARGS += -DCMAKE_BUILD_TYPE=Release
do-release: do-build

################################################################################

do-build:
	mkdir -p _build
	\[ -f _build/Makefile \] || (cd _build && cmake $(CMAKE_ARGS) ..)
	cmake --build _build

flash: force
	@\[ -f _build/stm32stand.elf \] || (echo 'No build found' && exit 1)
	$(stm32tools)/bin/arm-none-eabi-objcopy -O binary _build/stm32stand.elf stm32stand.bin
	@st-flash --reset write stm32stand.bin 0x08000000

flash-jlink: force
	@\[ -f _build/stm32stand.elf \] || (echo 'No build found' && exit 1)
	@JLinkExe -commanderscript flash.jlink

clean: force
	rm -rf _build

force:
