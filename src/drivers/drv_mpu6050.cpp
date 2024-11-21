#include "common.h"

#include "drivers.hpp"

namespace {

class driver : public Runnable, public MspDriver, public MspIrqHandler {
	I2C_HandleTypeDef *_h;

	static const uint16_t _address = (0x68 << 1);

	int read(uint8_t reg, uint8_t *ptr) {
		return read_bytes(reg, ptr, 1);
	}

	int read_bytes(uint8_t reg, uint8_t *ptr, uint16_t len) {
		return HAL_I2C_Mem_Read(_h, _address, reg, I2C_MEMADD_SIZE_8BIT, ptr, len, HAL_MAX_DELAY);
	}

	int write(uint8_t reg, uint8_t val) {
		return write_bytes(reg, &val, 1);
	}

	int write_bytes(uint8_t reg, uint8_t *ptr, uint16_t len) {
		return HAL_I2C_Mem_Write(_h, _address, reg, I2C_MEMADD_SIZE_8BIT, ptr, len, HAL_MAX_DELAY);
	}

	int write_and_check(uint8_t reg, uint8_t val) {
		if (write(reg, val) == HAL_OK) {
			uint8_t newval = -1;
			if (read(reg, &newval) == HAL_OK) {
				if (val == newval)
					return HAL_OK;
			}
		} return HAL_ERROR;
	}

	long do_init(void) {
		_h->Init.ClockSpeed = 400000;
		_h->Init.DutyCycle = I2C_DUTYCYCLE_2;
		_h->Init.OwnAddress1 = 0;
		_h->Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
		_h->Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
		_h->Init.OwnAddress2 = 0;
		_h->Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
		_h->Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
		if (HAL_I2C_Init(_h) != HAL_OK) {
			return HAL_ERROR;
		}

		static struct {
			uint8_t reg;
			uint8_t val;
		} configuration[] = {
			{ 0x75, 0x68 }, // whoami
			{ 0x6B, 0x00 },
			{ 0x19, 0x07 }, // SMPRT_DIV = 7
			{ 0x23, 0x00 }, // FIFO_EN = 0
			{ 0x38, 0x00 }, // INT_ENABLE = 0
			{ 0x1B, 0x00 }, // GYRO_CONFIG = FS_SEL(0)
			{ 0x1C, 0x00 }, // ACCEL_CONFIG = AFS_SEL(0)
		};

		for (size_t i = 0; i < ARRAY_SIZE(configuration); i++) {
			if (write_and_check(configuration[i].reg, configuration[i].val) != HAL_OK) {
				return HAL_ERROR;
			}
		}

		return HAL_OK;
	}

	long do_msp_init(void) {
		GPIO_InitTypeDef gpio = { 0 };	

		__HAL_RCC_GPIOA_CLK_ENABLE();

		gpio.Pin = GPIO_PIN_8;
		gpio.Mode = GPIO_MODE_AF_OD;
		gpio.Pull = GPIO_NOPULL;
		gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
		gpio.Alternate = GPIO_AF4_I2C3;
		HAL_GPIO_Init(GPIOA, &gpio);

		__HAL_RCC_GPIOC_CLK_ENABLE();

		gpio.Pin = GPIO_PIN_9;
		gpio.Mode = GPIO_MODE_AF_OD;
		gpio.Pull = GPIO_NOPULL;
		gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
		gpio.Alternate = GPIO_AF4_I2C3;
		HAL_GPIO_Init(GPIOC, &gpio);

		__HAL_RCC_I2C3_CLK_ENABLE();

		HAL_NVIC_SetPriority(I2C3_EV_IRQn, 0, 0);
		HAL_NVIC_EnableIRQ(I2C3_EV_IRQn);
		HAL_NVIC_SetPriority(I2C3_ER_IRQn, 0, 0);
		HAL_NVIC_EnableIRQ(I2C3_ER_IRQn);

		return HAL_OK;
	}

public:
	driver(I2C_HandleTypeDef *h) : _h(h) {}

	long init(void) {
		return do_init();
	}

	long mspInit(Handle *h) {
		if (h->Instance == _h->Instance) {
			return do_msp_init();
		} return HAL_OK;
	}

	void mspIrqHandler(IRQn_Type irq) {
		switch (irq) {
		case I2C3_EV_IRQn:
			HAL_I2C_EV_IRQHandler(_h);
			break;
		case I2C3_ER_IRQn:
			HAL_I2C_ER_IRQHandler(_h);
			break;
		default:
			break;
		}
	}

	void run(uint32_t millis) {
		int16_t gyro[ 3 ] = { -1, -1, -1 };
		read_bytes(0x43, (uint8_t *)gyro, sizeof(gyro));

		// atomic update with sign extension
#define int16to32(n) ((int32_t)((int16_t)(n)))
		__disable_irq();
		telemetry_set(GyroX, int16to32(tu_htons(gyro[0])));
		telemetry_set(GyroY, int16to32(tu_htons(gyro[1])));
		telemetry_set(GyroZ, int16to32(tu_htons(gyro[2])));
		__enable_irq();
# undef int16to32

		Runnable::arm(HAL_GetTick() + 10);
	}
};

} // namespace

Driver *mpu6050() {
	static I2C_HandleTypeDef h = { .Instance = I2C3 };
	static driver drv(&h);
	return &drv; // singleton
}
