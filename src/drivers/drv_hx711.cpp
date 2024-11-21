#include "common.h"

#include "drivers.hpp"


#define HX711_SCK_PIN GPIO_PIN_9        // Общий SCK пин для всех датчиков
#define HX711_DIN1_PIN GPIO_PIN_10      // DIN для первого датчика
#define HX711_DIN2_PIN GPIO_PIN_11      // DIN для второго датчика
#define HX711_DIN3_PIN GPIO_PIN_12      // DIN для третьего датчика
#define HX711_PORT GPIOE                // Порт для всех пинов

namespace {
    class driver : public Runnable, public MspDriver {

        static const int HX711_GAIN_A_128 = 1; // 25 импульсов

        int _gain, _bias1 = 0, _bias2 = 0, _bias3 = 0;
        int _lastval1 = 0, _lastval2 = 0, _lastval3 = 0;

        // Чтение данных с указанного DIN пина
        uint32_t read(uint16_t DIN_PIN) {
            uint32_t v = 0;
            __disable_irq();

            for (int i = 0; i < 24; i++) {
                HAL_GPIO_WritePin(HX711_PORT, HX711_SCK_PIN, GPIO_PIN_SET);
                HAL_GPIO_WritePin(HX711_PORT, HX711_SCK_PIN, GPIO_PIN_RESET);
                v = (v << 1) | HAL_GPIO_ReadPin(HX711_PORT, DIN_PIN);
            }

            for (int i = 0; i < _gain; i++) {
                HAL_GPIO_WritePin(HX711_PORT, HX711_SCK_PIN, GPIO_PIN_SET);
                HAL_GPIO_WritePin(HX711_PORT, HX711_SCK_PIN, GPIO_PIN_RESET);
            }

            __enable_irq();
            return ((v & 0x00800000) ? (v | 0xff000000) : v);
        }

        // Инициализация всех пинов
        long do_init(void) {
            GPIO_InitTypeDef gpio = { 0 };

            __HAL_RCC_GPIOE_CLK_ENABLE(); // Активируем порт E

            gpio.Pin = HX711_DIN1_PIN | HX711_DIN2_PIN | HX711_DIN3_PIN;
            gpio.Mode = GPIO_MODE_INPUT;
            gpio.Pull = GPIO_NOPULL;
            gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
            HAL_GPIO_Init(HX711_PORT, &gpio);

            gpio.Pin = HX711_SCK_PIN; // Настраиваем общий SCK как output
            gpio.Mode = GPIO_MODE_OUTPUT_PP;
            gpio.Pull = GPIO_NOPULL;
            gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
            HAL_GPIO_Init(HX711_PORT, &gpio);

            // Сброс SCK
            HAL_GPIO_WritePin(HX711_PORT, HX711_SCK_PIN, GPIO_PIN_RESET);

            return HAL_OK;
        }

    public:
        driver() : _gain(HX711_GAIN_A_128) {}

        long init(void) {
            return do_init();
        }

        long mspInit(Handle *) {
            return HAL_OK;
        }

        void tare(void) {
            _bias1 = _lastval1;
            _bias2 = _lastval2;
            _bias3 = _lastval3;
        }

        // Опрос всех датчиков
        void run(uint32_t millis) {
            if (HAL_GPIO_ReadPin(HX711_PORT, HX711_DIN1_PIN) == GPIO_PIN_RESET) {
                _lastval1 = read(HX711_DIN1_PIN);
                telemetry_set(Load1, _lastval1 - _bias1);
            }
            if (HAL_GPIO_ReadPin(HX711_PORT, HX711_DIN2_PIN) == GPIO_PIN_RESET) {
                _lastval2 = read(HX711_DIN2_PIN);
                telemetry_set(Load2, _lastval2 - _bias2);
            }
            if (HAL_GPIO_ReadPin(HX711_PORT, HX711_DIN3_PIN) == GPIO_PIN_RESET) {
                _lastval3 = read(HX711_DIN3_PIN);
                telemetry_set(Load3, _lastval3 - _bias3);
            }
            Runnable::arm();
        }
    };

} // namespace

Driver *hx711() {
    static driver drv;
    return &drv; // singleton
}
