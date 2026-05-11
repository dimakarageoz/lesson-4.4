#ifndef _SPI_MODULE_H_
#define _SPI_MODULE_H_

#include "esp_err.h"
#include "driver/spi_master.h"

#ifdef __cplusplus
extern "C" {
#endif

// Function to initialize the SPI bus and device
esp_err_t spi_module_init(spi_host_device_t host_id);

// Function to perform an SPI transaction with mutex protection
esp_err_t spi_module_transmit_receive(const uint8_t *tx_data, uint8_t *rx_data, size_t len);

#ifdef __cplusplus
}
#endif

#endif // _SPI_MODULE_H_