// Copyright (c) Kuba Szczodrzyński 2025-9-9.

#include "sd_priv.h"

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

	sd->use_dma	  = false;
	sd->tx_dma	  = dma_claim_unused_channel(true);
	sd->rx_dma	  = dma_claim_unused_channel(true);
	sd->tx_config = dma_channel_get_default_config(sd->tx_dma);
	sd->rx_config = dma_channel_get_default_config(sd->rx_dma);

	channel_config_set_transfer_data_size(&sd->tx_config, DMA_SIZE_8);
	channel_config_set_transfer_data_size(&sd->rx_config, DMA_SIZE_8);
	channel_config_set_dreq(&sd->tx_config, spi_get_dreq(sd->priv, true));
	channel_config_set_dreq(&sd->rx_config, spi_get_dreq(sd->priv, false));
	channel_config_set_sniff_enable(&sd->tx_config, true);
	channel_config_set_sniff_enable(&sd->rx_config, true);
	channel_config_set_write_increment(&sd->tx_config, false);
	channel_config_set_read_increment(&sd->rx_config, false);
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
	FOR_TIMEOUT(timeout) {
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
	uint8_t crc[2];

	FOR_TIMEOUT(100) {
		spi_read_blocking(sd->priv, 0xFF, data, 1);
		if (data[0] != 0xFF) {
			if (data[0] != CTRL_TOKEN_START)
				return SD_ERR_BAD_TOKEN;
			if (!sd->use_dma) {
				spi_read_blocking(sd->priv, 0xFF, data, len);
				spi_read_blocking(sd->priv, 0xFF, crc, 2);
			} else {
				channel_config_set_read_increment(&sd->tx_config, false);
				channel_config_set_write_increment(&sd->rx_config, true);
				static uint8_t fill = 0xFF;
				dma_channel_configure(sd->tx_dma, &sd->tx_config, &spi0_hw->dr, &fill, len, false);
				dma_channel_configure(sd->rx_dma, &sd->rx_config, data, &spi0_hw->dr, len, false);
				dma_sniffer_enable(sd->rx_dma, DMA_SNIFF_CTRL_CALC_VALUE_CRC16, false);
				dma_sniffer_set_data_accumulator(0);
				dma_start_channel_mask((1 << sd->tx_dma) | (1 << sd->rx_dma));
				dma_channel_wait_for_finish_blocking(sd->tx_dma);
				dma_channel_wait_for_finish_blocking(sd->rx_dma);
				spi_read_blocking(sd->priv, 0xFF, crc, 2);
				uint32_t crc_value = (crc[0] << 8) | (crc[1] << 0);
				if (crc_value != dma_sniffer_get_data_accumulator())
					return SD_ERR_BAD_CRC;
			}
			return SD_ERR_OK;
		}
	}
	return SD_ERR_DATA_TIMEOUT;
}
