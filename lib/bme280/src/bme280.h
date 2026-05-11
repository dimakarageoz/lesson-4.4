#include "driver/spi_master.h"

// Структура для калибровочных параметров
typedef struct {
    uint16_t dig_T1;
    int16_t  dig_T2;
    int16_t  dig_T3;

    uint16_t  dig_P9;
    int16_t  dig_P8;
    int16_t  dig_P7;
    int16_t  dig_P6;
    int16_t  dig_P5;
    int16_t  dig_P4;
    int16_t  dig_P3;
    int16_t  dig_P2;
    int16_t  dig_P1;

    uint16_t  dig_H1;
    int16_t  dig_H2;
    uint16_t  dig_H3;
    int16_t  dig_H4;
    int16_t  dig_H5;
    int16_t  dig_H6;
} bme280_calib_t;

int32_t BME280_setup_compensate(spi_device_handle_t spi_handle, bme280_calib_t *calib);

int32_t BME280_compensate_T_int32(bme280_calib_t *calib, int32_t adc_T);

int64_t BME280_compensate_P_int64(bme280_calib_t *calib, int32_t adc_P);

uint32_t bme280_compensate_H_int32(bme280_calib_t *calib, int32_t adc_H);

float bme280_pressure_butify(uint32_t raw_pressure);
float bme280_humidity_butify(int64_t raw_humidity);
float bme280_temperature_butify(int32_t raw_temperature);
