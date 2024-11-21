#include "common.h"
#include "drivers.hpp"

#include <algorithm>

namespace {

    class driver : public Motor {
        TIM_HandleTypeDef *_h;

        int _pulseWidth;
        int _pulseWidthTarget;

        const int _pulseWidthMin = 750;
        const int _pulseWidthMax = 2000;

        long do_init(void) {
            _h->Init.Prescaler = (168 - 1); // FIXME: dynamic -- to get 1MHz (1us)
            _h->Init.CounterMode = TIM_COUNTERMODE_UP;
            _h->Init.Period = (20000 - 1);
            _h->Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
            _h->Init.RepetitionCounter = 0;
            _h->Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
            if (HAL_TIM_PWM_Init(_h) != HAL_OK) {
                return HAL_ERROR;
            }

            TIM_ClockConfigTypeDef clockConfig = {0};
            clockConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
            if (HAL_TIM_ConfigClockSource(_h, &clockConfig) != HAL_OK) {
                return HAL_ERROR;
            }

            TIM_OC_InitTypeDef channelConfig = {0};
            channelConfig.OCMode = TIM_OCMODE_PWM1;
            channelConfig.Pulse = 0;
            channelConfig.OCPolarity = TIM_OCPOLARITY_HIGH;
            channelConfig.OCNPolarity = TIM_OCNPOLARITY_HIGH;
            channelConfig.OCFastMode = TIM_OCFAST_DISABLE;
            channelConfig.OCIdleState = TIM_OCIDLESTATE_RESET;
            channelConfig.OCNIdleState = TIM_OCNIDLESTATE_RESET;
            if (HAL_TIM_PWM_ConfigChannel(_h, &channelConfig, TIM_CHANNEL_2) != HAL_OK) {
                return HAL_ERROR;
            } else if (HAL_TIM_PWM_Start(_h, TIM_CHANNEL_2) != HAL_OK) {
                return HAL_ERROR;
            }

            // Инициализация капризных esc
            for (int pwm = 750; pwm <= 850; pwm += 1) {
                pulseWidthUpdate(pwm, false);
                HAL_Delay(10);
            }

            return 0;
        }

        long do_msp_init(void) {
            GPIO_InitTypeDef gpio = {0};

            __HAL_RCC_GPIOC_CLK_ENABLE(); // PC7

            gpio.Pin = GPIO_PIN_7;
            gpio.Mode = GPIO_MODE_AF_PP;
            gpio.Pull = GPIO_NOPULL;
            gpio.Speed = GPIO_SPEED_FREQ_LOW;
            gpio.Alternate = GPIO_AF3_TIM8;
            HAL_GPIO_Init(GPIOC, &gpio);

            __HAL_RCC_TIM8_CLK_ENABLE();

            return HAL_OK;
        }

        void pulseWidthUpdate(int us, bool retarget = true) {
            _pulseWidth = std::clamp(us, _pulseWidthMin, _pulseWidthMax);
            __HAL_TIM_SET_COMPARE(_h, TIM_CHANNEL_2, (uint32_t)_pulseWidth);
            telemetry_set(MotorThrottle, _pulseWidth);
            if (retarget) _pulseWidthTarget = _pulseWidth;
        }

        bool targeting() {
            return _pulseWidth != _pulseWidthTarget;
        }

    public:
        driver(TIM_HandleTypeDef *h) : _h(h) {}

        long init(void) {
            return do_init();
        }

        long mspInit(Handle *h) {
            if (h->Instance == _h->Instance) {
                return do_msp_init();
            }
            return HAL_OK;
        }

        void run(uint32_t millis) {
            if (targeting()) {
                // slowdown
                pulseWidthUpdate(std::clamp(_pulseWidth - 50, _pulseWidthMin, _pulseWidthMax), false);
                Runnable::arm(HAL_GetTick() + 200);
            } else {
                Runnable::arm();
            }
        }

        void throttle(int us) {
            if (targeting()) {
                return;
            } else if (us > 0) {
                pulseWidthUpdate(us);
            } else {
                _pulseWidthTarget = _pulseWidthMin;
            }
        }
    };

} // namespace

Motor *motor(void) {
    static TIM_HandleTypeDef h = {.Instance = TIM8};
    static driver drv(&h);
    return &drv;
}