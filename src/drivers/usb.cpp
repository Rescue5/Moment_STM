#include "common.h"

#include "drivers.hpp"

namespace {

class driver : public TinyUSB {

	static const int TelemetryChannelText = 0;
	static const int TelemetryChannelData = 1;

	long do_init(void) {
		GPIO_InitTypeDef gpio = { 0 };

		__HAL_RCC_GPIOA_CLK_ENABLE();

		//
		// NOTE: We use that code to reset USB on RESET so it'll be
		//       possible to open ttyACM right after flashing
		//

		gpio.Pin = GPIO_PIN_11 | GPIO_PIN_12;
		gpio.Mode = GPIO_MODE_OUTPUT_PP;
		gpio.Pull = GPIO_NOPULL;
		HAL_GPIO_Init(GPIOA, &gpio);

		//
		// 250ms delay is enough to wait after USB reset
		//

		HAL_Delay(250);

		// USB_OTG_FS GPIO Configuration
		// -----------------------------
		//   PA11 => USB_OTG_FS_DM
		//   PA12 => USB_OTG_FS_DP

		gpio.Pin = GPIO_PIN_11 | GPIO_PIN_12;
		gpio.Mode = GPIO_MODE_AF_PP;
		gpio.Pull = GPIO_NOPULL;
		gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
		gpio.Alternate = GPIO_AF10_OTG_FS;
		HAL_GPIO_Init(GPIOA, &gpio);

		// Peripheral clock enable
		__HAL_RCC_USB_OTG_FS_CLK_ENABLE();

		HAL_NVIC_SetPriority(OTG_FS_IRQn, 0, 0);
		HAL_NVIC_EnableIRQ(OTG_FS_IRQn);

		// STM32F407VGT6 doesn't use VBUS sense (B device) explicitly disable it
		USB_OTG_FS->GCCFG |= USB_OTG_GCCFG_NOVBUSSENS;
		USB_OTG_FS->GCCFG &= ~USB_OTG_GCCFG_VBUSBSEN;
		USB_OTG_FS->GCCFG &= ~USB_OTG_GCCFG_VBUSASEN;

		return HAL_OK;
	}

	void cdc_task(void) {
		uint8_t data[ 64 ];

		if (!tud_cdc_connected())
			return;

		uint32_t available = tud_cdc_available();
		while (available) {
			uint32_t size = tud_cdc_read(data, TU_MIN(sizeof(data), available));
			rx_bytes(data, size);
			available -= size;
		}
	}

	int write_raw(const uint8_t *ptr, size_t len) {
		int tries = 10;

		while (tries && len) {
			if (!tud_cdc_connected()) {
				tud_cdc_write_clear();
				return -1; // error while writing
			} else if (!tud_cdc_write_available()) {
				tud_cdc_write_flush();
				tud_task(), tries--;
				continue;
			}

			size_t nbytes_written = tud_cdc_write(ptr, len);
			if (!nbytes_written) {
				tud_cdc_write_flush();
				tud_task(), tries--;
				continue;
			} else {
				ptr += nbytes_written;
				len -= nbytes_written;
			}

			tud_cdc_write_flush();
		}

		return tries ? 0 : -1;
	}

	int write(int ch, const void *ptr, size_t len) {
		uint32_t clen = (ch << 24) | len;
		uint32_t csum = 0; // TODO: checksum support

		if ((ch < 0 || ch > 7) || len > 0xffff)
			return -1;

		uint32_t syn[] = { 0xc1c1c1c1, clen };
		uint32_t fin[] = { csum, 0xc2c2c2c2 };

		// TODO: add locking
	
		if (write_raw((uint8_t *)syn, sizeof(syn)) ||
		    write_raw((uint8_t *)ptr, len) ||
		    write_raw((uint8_t *)fin, sizeof(fin)))
			return -1;

		return 0;
	}

public:
	long init(void) {
		long res = do_init();
		if (res == HAL_OK)
			tud_init(BOARD_TUD_RHPORT);
		return res;
	}

	long mspInit(Handle *) {
		return HAL_OK;
	}

	void write(const char *str) {
		write(TelemetryChannelText, (const void *)str, strlen(str));
	}

	void write(const void *ptr, size_t len) {
		write(TelemetryChannelData, (const void *)ptr, len);
	}

	void run(uint32_t millis) {
		tud_task();
		cdc_task();
		Runnable::arm();
	}
};


} // namespace

TinyUSB *tinyUSB(void) {
	static driver drv;
	return &drv; // singleton
}
