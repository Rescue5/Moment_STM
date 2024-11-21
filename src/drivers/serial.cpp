#include "common.h"

#include "drivers.hpp"

namespace {

class driver : public Serial {
	UART_HandleTypeDef *_h;

	long do_init(void) {
		_h->Init.BaudRate = 115200;
		_h->Init.WordLength = UART_WORDLENGTH_8B;
		_h->Init.StopBits = UART_STOPBITS_1;
		_h->Init.Parity = UART_PARITY_NONE;
		_h->Init.Mode = UART_MODE_TX_RX;
		_h->Init.HwFlowCtl = UART_HWCONTROL_NONE;
		_h->Init.OverSampling = UART_OVERSAMPLING_16;
		if (HAL_UART_Init(_h) != HAL_OK) {
			return HAL_ERROR;
		}

		setvbuf(stdout, NULL, _IONBF, 0); // disable printf() buffering

		return HAL_OK;
	}

	long do_msp_init(void) {
		GPIO_InitTypeDef gpio = { 0 };

		__HAL_RCC_GPIOD_CLK_ENABLE();

		gpio.Pin = GPIO_PIN_8 | GPIO_PIN_9;
		gpio.Mode = GPIO_MODE_AF_PP;
		gpio.Pull = GPIO_NOPULL;
		gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
		gpio.Alternate = GPIO_AF7_USART3;
		HAL_GPIO_Init(GPIOD, &gpio);

		__HAL_RCC_USART3_CLK_ENABLE();

		return HAL_OK;
	}

public:
	driver(UART_HandleTypeDef *h) : _h(h) {}

	long init(void) {
		return do_init();
	}

	long mspInit(Handle *h) {
		if (h->Instance == _h->Instance) {
			return do_msp_init();
		} return HAL_OK;
	}

	void run(uint32_t millis) {
		// FIXME
	}

	void putchar(int ch) {
		HAL_UART_Transmit(_h, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
	}
};

} // namespace

Serial *serial(void) {
	static UART_HandleTypeDef h = { .Instance = USART3 };
	static driver drv(&h);
	return &drv;
}

extern "C" {

ssize_t _write(int file, char *ptr, size_t len) {
#ifndef NDEBUG
	for (size_t i = 0; i < len; i++) {
		serial()->putchar(*ptr++);
	}
#endif
	return len;
}

} // extern "C"
