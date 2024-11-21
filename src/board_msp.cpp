#include "common.h"

#include "drivers.hpp"

volatile uint32_t uptimeMillis = 0;

// https://electronics.stackexchange.com/questions/521020/stm32-create-a-microsecond-timer/523043#523043
uint32_t GetMicros(void) {
	uint32_t ms;
	uint32_t st;

	do {
		ms = uptimeMillis;
		st = SysTick->VAL;
		__NOP();
		__NOP();
	} while (ms != uptimeMillis);

	return ms * 1000 - (st * 1000) / (SysTick->LOAD + 1);
}

extern "C" { // do not remove

void HAL_MspInit(void) {
	__HAL_RCC_PWR_CLK_ENABLE();
	__HAL_RCC_SYSCFG_CLK_ENABLE();

	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOD_CLK_ENABLE();
	__HAL_RCC_GPIOE_CLK_ENABLE();
}

void HAL_SPI_MspInit(SPI_HandleTypeDef *h) {
	driverManager()->msp_init((Handle *)h);
}

void HAL_I2C_MspInit(I2C_HandleTypeDef *h) {
	driverManager()->msp_init((Handle *)h);
}

void HAL_UART_MspInit(UART_HandleTypeDef* h) {
	driverManager()->msp_init((Handle *)h);
}
	
void HAL_TIM_PWM_MspInit(TIM_HandleTypeDef* h) {
	driverManager()->msp_init((Handle *)h);
}
	
////////////////////////////////////////////////////////////////////////////////

void SysTick_Handler(void) {
	HAL_IncTick(), uptimeMillis++;
}

void SPI2_IRQHandler(void) {
	driverManager()->msp_irq_handler(SPI2_IRQn);
}

void I2C1_EV_IRQHandler(void) {
	driverManager()->msp_irq_handler(I2C1_EV_IRQn);
}

void I2C1_ER_IRQHandler(void) {
	driverManager()->msp_irq_handler(I2C1_ER_IRQn);
}

void I2C3_EV_IRQHandler(void) {
	driverManager()->msp_irq_handler(I2C3_EV_IRQn);
}

void I2C3_ER_IRQHandler(void) {
	driverManager()->msp_irq_handler(I2C3_ER_IRQn);
}

void EXTI0_IRQHandler(void) {
	HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_5); // PC0 -> speed
}
void EXTI9_5_IRQHandler(void) {
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_5); // PA5 -> speed
}

void OTG_FS_IRQHandler(void) {
	tud_int_handler(0);
}

////////////////////////////////////////////////////////////////////////////////

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *h) {
	driverManager()->msp_callback((void *)&HAL_I2C_ErrorCallback, (Handle *)h);
}

void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *h) {
	driverManager()->msp_callback((void *)&HAL_I2C_MasterTxCpltCallback, (Handle *)h);
}

void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *h) {
	driverManager()->msp_callback((void *)&HAL_I2C_MasterRxCpltCallback, (Handle *)h);
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *h) {
	driverManager()->msp_callback((void *)&HAL_SPI_ErrorCallback, (Handle *)h);
}

void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *h) {
	driverManager()->msp_callback((void *)&HAL_SPI_RxCpltCallback, (Handle *)h);
}

void HAL_GPIO_EXTI_Callback(uint16_t pin) {
	Handle h = { nullptr, { .Pin = pin } };
	driverManager()->msp_callback((void *)&HAL_GPIO_EXTI_Callback, (Handle *)&h);
}

////////////////////////////////////////////////////////////////////////////////

} // do not remove
