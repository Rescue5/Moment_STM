#include "common.h"

#include "drivers.hpp"

namespace {

class driver : public Runnable, public MspDriver, public MspCallback, public MspIrqHandler {
	I2C_HandleTypeDef *_h;

	int _step = 0;
	uint16_t _motorU, _motorP, _motorI;
	static const uint16_t _address = 0x40 << 1;

	long read(uint8_t reg, uint16_t *val) {
		if (HAL_I2C_Master_Transmit(_h, _address, &reg, 1, HAL_MAX_DELAY) != HAL_OK) {
			return HAL_ERROR;
		} else if (HAL_I2C_Master_Receive(_h, _address, (uint8_t *)val, sizeof(*val), HAL_MAX_DELAY) != HAL_OK) {
			return HAL_ERROR;
		} else {
			*val = tu_htons(*val);
			return HAL_OK;
		}
	}

	long write(uint8_t reg, uint16_t val) {
		uint8_t data[] = { reg, val >> 8, val & 0xff };
		return HAL_I2C_Master_Transmit(_h, _address, data, sizeof(data), HAL_MAX_DELAY);
	}

	long write_and_check(uint8_t reg, uint16_t val) {
		if (write(reg, val) == HAL_OK) {
			uint16_t newval = -1;
			if (read(reg, &newval) == HAL_OK) {
				if (val == newval)
					return HAL_OK;
			}
		} return HAL_ERROR;
	}

	long do_init(void) {
		_h->Init.ClockSpeed = 100000;
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
			uint8_t  reg;
			uint16_t val;
		} configuration[] = {
			{ 0x3E, 0x5449 }, // Manufacturer ID Register
			{ 0x3F, 0xA080 }, // Device ID Register
			{ 0x00, 0x4327 }, // Configuration Register
			{ 0x05, 512    }, // Calibration Register (SHUNT_CAL)
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

        __HAL_RCC_GPIOB_CLK_ENABLE();

        gpio.Pin = GPIO_PIN_8;  // Изменено на PB8
        gpio.Mode = GPIO_MODE_AF_OD;
        gpio.Pull = GPIO_NOPULL;
        gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        gpio.Alternate = GPIO_AF4_I2C1;
        HAL_GPIO_Init(GPIOB, &gpio);

        gpio.Pin = GPIO_PIN_9;  // Изменено на PB9
        gpio.Mode = GPIO_MODE_AF_OD;
        gpio.Pull = GPIO_NOPULL;
        gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        gpio.Alternate = GPIO_AF4_I2C1;
        HAL_GPIO_Init(GPIOB, &gpio);

        __HAL_RCC_I2C1_CLK_ENABLE();

        HAL_NVIC_SetPriority(I2C1_EV_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(I2C1_EV_IRQn);
        HAL_NVIC_SetPriority(I2C1_ER_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(I2C1_ER_IRQn);

        return HAL_OK;
    }

	void do_step(void) {
		int res = HAL_ERROR;

		static uint8_t regs[] = {
			0x02, // Bus Voltage Register
			0x03, // Power Register
			0x04, // Current Register
		};

		switch (_step++) {
		case 0:
			res = HAL_I2C_Master_Transmit_IT(_h, _address, &regs[0], 1);
			break;
		case 1:
			res = HAL_I2C_Master_Receive_IT(_h, _address, (uint8_t *)&_motorU, sizeof(_motorU));
			telemetry_set(MotorU, tu_htons(_motorU));
			break;
		case 2:
			res = HAL_I2C_Master_Transmit_IT(_h, _address, &regs[1], 1);
			break;
		case 3:
			res = HAL_I2C_Master_Receive_IT(_h, _address, (uint8_t *)&_motorP, sizeof(_motorP));
			telemetry_set(MotorP, tu_htons(_motorP));
			break;
		case 4:
			res = HAL_I2C_Master_Transmit_IT(_h, _address, &regs[2], 1);
			break;
		case 5:
			res = HAL_I2C_Master_Receive_IT(_h, _address, (uint8_t *)&_motorI, sizeof(_motorI));
			telemetry_set(MotorI, tu_htons(_motorI));
			break;
		}

		if (res != HAL_OK) rearm();
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

	void mspCallback(void *p, Handle *h) {
		if (h->Instance == _h->Instance) {
			if ((void *)&HAL_I2C_ErrorCallback == p) {
				rearm();
			} else if ((void *)&HAL_I2C_MasterTxCpltCallback == p) {
				do_step();
			} else if ((void *)&HAL_I2C_MasterRxCpltCallback == p) {
				do_step();
			}
		}
	}

	void mspIrqHandler(IRQn_Type irq) {
		switch (irq) {
		case I2C1_EV_IRQn:
			HAL_I2C_EV_IRQHandler(_h);
			break;
		case I2C1_ER_IRQn:
			HAL_I2C_ER_IRQHandler(_h);
			break;
		default:
			break;
		}
	}

	void rearm() {
		Runnable::arm(HAL_GetTick() + 10), _step = 0;
	}

	void run(uint32_t millis) {
		do_step();
	}
};

} // namespace
	
Driver *ina236() {
	static I2C_HandleTypeDef h = { .Instance = I2C1 };
	static driver drv(&h);
	return &drv; // singleton
}
