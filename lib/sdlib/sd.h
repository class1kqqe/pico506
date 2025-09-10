// Copyright (c) Kuba Szczodrzyński 2025-9-9.

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <hardware/clocks.h>
#include <hardware/dma.h>
#include <hardware/spi.h>
#include <pico/stdlib.h>
#include <pico/time.h>

typedef struct sd_t sd_t;

typedef enum {
	SD_ERR_OK = 0,
	SD_ERR_CMD_TIMEOUT,
	SD_ERR_DATA_TIMEOUT,
	SD_ERR_BUSY_TIMEOUT,
	SD_ERR_BAD_R1,
	SD_ERR_BAD_TOKEN,
	SD_ERR_INIT_IDLE_FAILED,
	SD_ERR_INIT_READY_FAILED,
	SD_ERR_INIT_BAD_ECHO,
	SD_ERR_INIT_POWER_UP_BUSY,
} sd_err_t;

typedef void (*sd_power_on_t)(sd_t *sd);
typedef void (*sd_command_t)(sd_t *sd, uint8_t cmd, uint32_t arg, uint8_t crc);
typedef sd_err_t (*sd_wait_busy_t)(sd_t *sd, uint8_t *resp, uint32_t timeout);
typedef sd_err_t (*sd_read_r1_t)(sd_t *sd, uint8_t *resp);
typedef sd_err_t (*sd_read_rx_t)(sd_t *sd, uint8_t *resp, uint32_t len);
typedef sd_err_t (*sd_read_data_t)(sd_t *sd, uint8_t *resp, uint32_t len);

typedef struct sd_t {
	bool init_ok;
	bool ccs;
	uint8_t csd[16];
	uint8_t cid[16];

	char name[6];
	uint32_t capacity_kb;

	sd_power_on_t power_on;
	sd_command_t command;
	sd_wait_busy_t wait_busy;
	sd_read_r1_t read_r1;
	sd_read_rx_t read_rx;
	sd_read_data_t read_data;

	void *priv;
	uint cs_pin;
} sd_t;

// sd.c
sd_err_t sd_init(sd_t *sd);

// sd_spi.c
void sd_spi_init(sd_t *sd, uint32_t freq, uint sck, uint tx, uint rx, uint cs);

// sd_debug.c
void sd_print_r1(const uint8_t *resp, bool no_bytes);
void sd_print_r3(const uint8_t *resp);
void sd_print_r7(const uint8_t *resp);
