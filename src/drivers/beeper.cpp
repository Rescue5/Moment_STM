#include "common.h"

#include "drivers.hpp"

namespace {

class driver : public Beeper {
	TIM_HandleTypeDef *_h;

	long do_init(void) {
		_h->Init.Prescaler = (168-1); // FIXME: dynamic -- to get 1MHz (1us)
		_h->Init.CounterMode = TIM_COUNTERMODE_UP;
		_h->Init.Period = (1000-1);
		_h->Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
		_h->Init.RepetitionCounter = 0;
		_h->Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
		if (HAL_TIM_PWM_Init(_h) != HAL_OK) {
			return HAL_ERROR;
		}

		TIM_ClockConfigTypeDef clockConfig = { 0 };
		clockConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
		if (HAL_TIM_ConfigClockSource(_h, &clockConfig) != HAL_OK) {
			return HAL_ERROR;
		}

		TIM_OC_InitTypeDef channelConfig = { 0 };
		channelConfig.OCMode = TIM_OCMODE_PWM1;
		channelConfig.Pulse = 10; // fixed tone
		channelConfig.OCPolarity = TIM_OCPOLARITY_HIGH;
		channelConfig.OCNPolarity = TIM_OCNPOLARITY_HIGH;
		channelConfig.OCFastMode = TIM_OCFAST_DISABLE;
		channelConfig.OCIdleState = TIM_OCIDLESTATE_RESET;
		channelConfig.OCNIdleState = TIM_OCNIDLESTATE_RESET;
		if (HAL_TIM_PWM_ConfigChannel(_h, &channelConfig, TIM_CHANNEL_1) != HAL_OK) {
			return HAL_ERROR;
		}

		return HAL_OK;
	}

	long do_msp_init(void) {
		GPIO_InitTypeDef gpio = { 0 };

		__HAL_RCC_GPIOB_CLK_ENABLE(); // PB8

		gpio.Pin = GPIO_PIN_8;
		gpio.Mode = GPIO_MODE_AF_PP;
		gpio.Pull = GPIO_NOPULL;
		gpio.Speed = GPIO_SPEED_FREQ_LOW;
		gpio.Alternate = GPIO_AF3_TIM10;
		HAL_GPIO_Init(GPIOB, &gpio);

		__HAL_RCC_TIM10_CLK_ENABLE();

		return HAL_OK;
	}

	void pwmON(void) {
		HAL_TIM_PWM_Start(_h, TIM_CHANNEL_1);
	}

	void pwmOFF(void) {
		HAL_TIM_PWM_Stop(_h, TIM_CHANNEL_1);
	}

public:
	driver(TIM_HandleTypeDef *h) : _h(h) {}

	long init(void) {
		return do_init();
	}

	long mspInit(Handle *h) {
		if (h->Instance == _h->Instance) {
			return do_msp_init();
		} return HAL_OK;
	}

	void run(uint32_t millis) {
		Runnable::arm();
	}

	void disco(void) {
		for (;;) {
			uint32_t tick = HAL_GetTick();
			if ((tick % 133) == 0) pwmON();
			else if ((tick % 189) == 0) pwmOFF();
		}
	}
};

} // namespace

Beeper *beeper(void) {
	static TIM_HandleTypeDef h = { .Instance = TIM10 };
	static driver drv(&h);
	return &drv;
}
