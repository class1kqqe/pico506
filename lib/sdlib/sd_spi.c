// Copyright (c) Kuba Szczodrzyński 2025-9-9.

#include "sd.h"
#include "sd_defs.h"

static void sd_spi_power_on(sd_t *sd);
static void sd_spi_command(sd_t *sd, uint8_t cmd, uint32_t arg, uint8_t crc);
static sd_err_t sd_spi_wait_busy(sd_t *sd, uint8_t *resp, uint32_t timeout);
static sd_err_t sd_spi_read_r1(sd_t *sd, uint8_t *resp);
static sd_err_t sd_spi_read_rx(sd_t *sd, uint8_t *resp, uint32_t len);
static sd_err_t sd_spi_read_data(sd_t *sd, uint8_t *data, uint32_t len);

void sd_spi_init(sd_t *sd, uint32_t freq, uint sck, uint tx, uint rx, uint cs) {
	sd->power_on  = sd_spi_power_on;
	sd->command	  = sd_spi_command;
	sd->wait_busy = sd_spi_wait_busy;
	sd->read_r1	  = sd_spi_read_r1;
	sd->read_rx	  = sd_spi_read_rx;
	sd->read_data = sd_spi_read_data;
	sd->priv	  = spi0;
	sd->cs_pin	  = cs;

	spi_init(sd->priv, freq);
	spi_set_format(sd->priv, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
	gpio_set_function(sck, GPIO_FUNC_SPI);
	gpio_set_function(tx, GPIO_FUNC_SPI);
	gpio_set_function(rx, GPIO_FUNC_SPI);
	gpio_set_function(cs, GPIO_FUNC_SPI);
	gpio_set_pulls(rx, true, false);
}

static void sd_spi_power_on(sd_t *sd) {
	uint8_t resp[10];
	gpio_set_function(sd->cs_pin, GPIO_FUNC_SIO);
	gpio_set_dir(sd->cs_pin, true);
	gpio_put(sd->cs_pin, 1);
	sleep_ms(10);
	spi_read_blocking(sd->priv, 0xFF, resp, 10);
	gpio_set_function(sd->cs_pin, GPIO_FUNC_SPI);
}

static void sd_spi_command(sd_t *sd, uint8_t cmd, uint32_t arg, uint8_t crc) {
	uint8_t buf[] = {
		0xFF,
		0x40 | cmd,
		arg >> 24,
		arg >> 16,
		arg >> 8,
		arg >> 0,
		crc | 0x01,
	};
	spi_write_blocking(sd->priv, buf, sizeof(buf));
}

static sd_err_t sd_spi_wait_busy(sd_t *sd, uint8_t *resp, uint32_t timeout) {
	for (uint32_t i = 0; i < timeout; i++) {
		spi_read_blocking(sd->priv, 0xFF, resp, 1);
		if (resp[0] == 0xFF)
			return SD_ERR_OK;
	}
	return SD_ERR_BUSY_TIMEOUT;
}

static sd_err_t sd_spi_read_r1(sd_t *sd, uint8_t *resp) {
	for (uint32_t i = 0; i < 8; i++) {
		spi_read_blocking(sd->priv, 0xFF, resp, 1);
		if (resp[0] != 0xFF) {
			if (resp[0] > 1)
				return SD_ERR_BAD_R1;
			return SD_ERR_OK;
		}
	}
	return SD_ERR_CMD_TIMEOUT;
}

static sd_err_t sd_spi_read_rx(sd_t *sd, uint8_t *resp, uint32_t len) {
	sd_err_t err;
	if ((err = sd_spi_read_r1(sd, resp)))
		return err;
	spi_read_blocking(sd->priv, 0xFF, &resp[1], len - 1);
	return SD_ERR_OK;
}

static sd_err_t sd_spi_read_data(sd_t *sd, uint8_t *data, uint32_t len) {
	uint8_t resp[1];
	uint8_t crc[2];

	for (uint32_t i = 0; i < 2000; i++) {
		spi_read_blocking(sd->priv, 0xFF, resp, 1);
		if (resp[0] != 0xFF) {
			if (resp[0] != CTRL_TOKEN_START)
				return SD_ERR_BAD_TOKEN;
			spi_read_blocking(sd->priv, 0xFF, data, len);
			spi_read_blocking(sd->priv, 0xFF, crc, 2);
			return SD_ERR_OK;
		}
	}
	return SD_ERR_DATA_TIMEOUT;
}
