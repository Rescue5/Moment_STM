#include "common.h"

#include "drivers.hpp"

namespace {

class driver : public Runnable, public MspDriver, public MspCallback, public MspIrqHandler {
	SPI_HandleTypeDef *_h;

	uint16_t _temp1 = -1, _temp2 = -1;

	long do_init(void) {
		_h->Init.Mode = SPI_MODE_MASTER; 
		_h->Init.Direction = SPI_DIRECTION_2LINES_RXONLY;
		_h->Init.DataSize = SPI_DATASIZE_16BIT;
		_h->Init.CLKPolarity = SPI_POLARITY_LOW;
		_h->Init.CLKPhase = SPI_PHASE_1EDGE;
		_h->Init.NSS = SPI_NSS_SOFT;
		_h->Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_128; // works for both sensors
		_h->Init.FirstBit = SPI_FIRSTBIT_MSB;
		_h->Init.TIMode = SPI_TIMODE_DISABLED;
		_h->Init.CRCCalculation = SPI_CRCCALCULATION_DISABLED;
		_h->Init.CRCPolynomial = 10;
		if (HAL_SPI_Init(_h) != HAL_OK) {
			return HAL_ERROR;
		}

		return HAL_OK;
	}

	long do_msp_init(void) {
		GPIO_InitTypeDef gpio = { 0 };

		__HAL_RCC_GPIOB_CLK_ENABLE(); // PB13, PB14 + PB6(CS_A)
		__HAL_RCC_GPIOD_CLK_ENABLE(); // PD6(CS_B)

		memset(&gpio, 0, sizeof(gpio));
		gpio.Pin = GPIO_PIN_13 | GPIO_PIN_14;
		gpio.Mode = GPIO_MODE_AF_PP;
		gpio.Pull = GPIO_NOPULL;
		gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
		gpio.Alternate = GPIO_AF5_SPI2;
		HAL_GPIO_Init(GPIOB, &gpio);

		__HAL_RCC_SPI2_CLK_ENABLE();

		memset(&gpio, 0, sizeof(gpio));
		gpio.Pin = GPIO_PIN_6;
		gpio.Mode = GPIO_MODE_OUTPUT_PP;
		gpio.Pull = GPIO_NOPULL;
		gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
		HAL_GPIO_Init(GPIOD, &gpio);
		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_6, GPIO_PIN_SET);

		memset(&gpio, 0, sizeof(gpio));
		gpio.Pin = GPIO_PIN_11;
		gpio.Mode = GPIO_MODE_OUTPUT_PP;
		gpio.Pull = GPIO_NOPULL;
		gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
		HAL_GPIO_Init(GPIOB, &gpio);
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, GPIO_PIN_SET);

		HAL_NVIC_SetPriority(SPI2_IRQn, 0, 0);
		HAL_NVIC_EnableIRQ(SPI2_IRQn);

		return HAL_OK;
	}

	void do_step(void) {
		int cs_a = HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_6);
		int cs_b = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_11);

		switch ((cs_a << 1 | cs_b)) {
		case 0b11:
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_6, GPIO_PIN_RESET);
			HAL_SPI_Receive_IT(_h, (uint8_t *)&_temp1, 1);
			break;
		case 0b01:
			if ((_temp1 >>= 3) != 0)
				telemetry_set(Temp1, _temp1);
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_6, GPIO_PIN_SET);
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, GPIO_PIN_RESET);
			HAL_SPI_Receive_IT(_h, (uint8_t *)&_temp2, 1);
			break;
		case 0b10:
			if ((_temp2 >>= 3) != 0)
				telemetry_set(Temp2, _temp2);
			[[fallthrough]];
		default:
			rearm();
		}
	}

public:
	driver(SPI_HandleTypeDef *h) : _h(h) {}

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
			if ((void *)*HAL_SPI_ErrorCallback == p) {
				rearm();
			} else if ((void *)&HAL_SPI_RxCpltCallback == p) {
				do_step();
			}
		}
	}

	void mspIrqHandler(IRQn_Type irq) {
		switch (irq) {
		case SPI2_IRQn:
			HAL_SPI_IRQHandler(_h);
			break;
		default:
			break;
		}
	}

	void rearm(void) {
		// force set (deactivate) ~CS for both chips
		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_6, GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, GPIO_PIN_SET);
		// NOTE: !!! as per the manual conversion time is 170-220ms
		Runnable::arm(HAL_GetTick() + 200);
	}

	void run(uint32_t millis) {
		do_step();
	}
};

} // namespace

Driver *max6675() {
	static SPI_HandleTypeDef h = { .Instance = SPI2 };
	static driver drv(&h);
	return &drv; // singleton
}
