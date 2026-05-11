#include <stdio.h>
#include <string.h>

#include "driver/spi_master.h"

void spi_write_reg(spi_device_handle_t spi_handle, uint8_t reg, uint8_t val) {
    uint8_t tx[] = { reg & 0x7F, val };

    spi_transaction_t t = {
        .length = 16,
        .tx_buffer = tx
    };
    
    spi_device_transmit(spi_handle, &t);
}

void spi_read_regs(spi_device_handle_t spi_handle, uint8_t reg, uint8_t *data, size_t len) {
    uint8_t tx[len + 1];
    uint8_t rx[len + 1];

    memset(tx, 0, sizeof(tx));

    tx[0] = reg | 0x80;
    
    spi_transaction_t t = {
        .length = (len + 1) * 8,
        .tx_buffer = tx,
        .rx_buffer = rx
    };

    spi_device_transmit(spi_handle, &t);
    
    for (size_t i = 0; i < len; i++) data[i] = rx[i + 1];
}
