#include <stdio.h>

#include "driver/spi_master.h"

void spi_write_reg(spi_device_handle_t spi_handle, uint8_t reg, uint8_t val);

void spi_read_regs(spi_device_handle_t spi_handle, uint8_t reg, uint8_t *data, size_t len);