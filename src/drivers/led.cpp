#include "common.h"

#include "drivers.hpp"

namespace {

class driver : public Led {
	unsigned int _mode = UINT_MAX;
	uint32_t _next_toggle = 0;

	void x_on(void) {
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);
	}

	void x_off(void) {
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);
	}

	void x_toggle(void) {
		HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_1);
	}

	long do_msp_init(void) {
		GPIO_InitTypeDef gpio = { 0 };

		__HAL_RCC_GPIOA_CLK_ENABLE();

		gpio.Pin = GPIO_PIN_1;
		gpio.Pull = GPIO_NOPULL;
		gpio.Mode = GPIO_MODE_OUTPUT_PP;
		gpio.Speed = GPIO_SPEED_FREQ_HIGH;
		HAL_GPIO_Init(GPIOA, &gpio);

		setMode(500);

		return HAL_OK;
	}

public:
	driver() {}

	long init(void) {
		return do_msp_init();
	}

	long mspInit(Handle *) {
		return HAL_OK;
	}

	void run(uint32_t millis) {
		switch (_mode) {
		case 0:
			x_off();
			break;
		case UINT_MAX:
			x_on();
			break;
		default:
			if (millis > _next_toggle) {
				x_toggle();
				_next_toggle = millis + _mode;
			}
			break;
		}
		Runnable::arm();
	}

	void setMode(unsigned int mode) {
		_mode = mode;
	}
};

}

Led *led() {
	static driver drv;
	return &drv;
}
