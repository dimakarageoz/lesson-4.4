#include <stdio.h>

#include "esp_log.h"

#include "bme280.h"
#include "spi.h"

extern char *TAG;

// Возвращает температуру в градусах Цельсия, разрешение 0.01 C.
// Выходное значение "5123" соответствует 51.23 C.
// t_fine хранит точное значение температуры как глобальную переменную
static int32_t t_fine;

int32_t BME280_setup_compensate(spi_device_handle_t spi_handle, bme280_calib_t *calib) {
    uint8_t cal_data[25];

    spi_read_regs(spi_handle, 0x88, cal_data, 25);

    calib->dig_T1 = (cal_data[1] << 8) | cal_data[0];
    calib->dig_T2 = (cal_data[3] << 8) | cal_data[2];
    calib->dig_T3 = (cal_data[5] << 8) | cal_data[4];

    calib->dig_P1 = (cal_data[7] << 8) | cal_data[6];
    calib->dig_P2 = (cal_data[9] << 8) | cal_data[8];
    calib->dig_P3 = (cal_data[11] << 8) | cal_data[10];
    calib->dig_P4 = (cal_data[13] << 8) | cal_data[12];
    calib->dig_P5 = (cal_data[15] << 8) | cal_data[14];
    calib->dig_P6 = (cal_data[17] << 8) | cal_data[16];
    calib->dig_P7 = (cal_data[19] << 8) | cal_data[18];
    calib->dig_P8 = (cal_data[21] << 8) | cal_data[20];
    calib->dig_P9 = (cal_data[23] << 8) | cal_data[22];

    calib->dig_H1 = cal_data[24];

    uint8_t cal_data_2[7];

    spi_read_regs(spi_handle, 0xE1, cal_data_2, 7);

    calib->dig_H2 = (cal_data_2[1] << 8) | cal_data_2[0];
    calib->dig_H3 = cal_data_2[2];
    calib->dig_H4 = (cal_data_2[3] << 4) | (cal_data_2[4] & 0x0F);
    calib->dig_H5 = (cal_data_2[5] << 4) | (cal_data_2[4] >> 4);
    calib->dig_H6 = cal_data_2[6];

    ESP_LOGI(TAG, "dig_T1=%u, dig_T2=%d, dig_T3=%d", calib->dig_T1, calib->dig_T2, calib->dig_T3);
    ESP_LOGI(TAG, "dig_P1=%u, dig_P2=%d, dig_P3=%d", calib->dig_P1, calib->dig_P2, calib->dig_P3);
    ESP_LOGI(TAG, "dig_P4=%d, dig_P5=%d, dig_P6=%d", calib->dig_P4, calib->dig_P5, calib->dig_P6);
    ESP_LOGI(TAG, "dig_P7=%d, dig_P8=%d, dig_P9=%d", calib->dig_P7, calib->dig_P8, calib->dig_P9);
    ESP_LOGI(TAG, "dig_H1=%u, dig_H2=%d, dig_H3=%d", calib->dig_H1, calib->dig_H2, calib->dig_H3);
    ESP_LOGI(TAG, "dig_H4=%d, dig_H5=%d, dig_H6=%d", calib->dig_H4, calib->dig_H5, calib->dig_H6);

    return 0;
}

int32_t BME280_compensate_T_int32(bme280_calib_t *calib, int32_t adc_T)
{
    int32_t var1, var2;
    var1 = ((((adc_T >> 3) - ((int32_t)calib->dig_T1 << 1))) * ((int32_t)calib->dig_T2)) >> 11;
    var2 = (((((adc_T >> 4) - ((int32_t)calib->dig_T1)) * ((adc_T >> 4) - ((int32_t)calib->dig_T1))) >> 12) *
            ((int32_t)calib->dig_T3)) >> 14;
    t_fine = var1 + var2;

    return (t_fine * 5 + 128) >> 8;
}

// Возвращает давление в Паскалях как 32-битное целое в формате Q24.8.
// Выходное значение "24674867" представляет 24674867/256 = 96386.2 Па = 963.862 гПа
int64_t BME280_compensate_P_int64(bme280_calib_t *calib, int32_t adc_P)
{
    int64_t var1, var2, p;
    var1 = ((int64_t)t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)calib->dig_P6;
    var2 = var2 + ((var1 * (int64_t)calib->dig_P5) << 17);
    var2 = var2 + (((int64_t)calib->dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)calib->dig_P3) >> 8) + ((var1 * (int64_t)calib->dig_P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)calib->dig_P1) >> 33;
    if (var1 == 0)
    {
        return 0; // избегаем деления на ноль
    }
    p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)calib->dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)calib->dig_P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((int64_t)calib->dig_P7) << 4);

    return p;
}

// Возвращает влажность в %RH как 32-битное целое в формате Q22.10.
// Выходное значение "47445" представляет 47445/1024 = 46.333 %RH
uint32_t bme280_compensate_H_int32(bme280_calib_t *calib, int32_t adc_H)
{
    int32_t v_x1_u32r;

    v_x1_u32r = (t_fine - ((int32_t)76800));

    v_x1_u32r = (
        ((((adc_H << 14) - (((int32_t)calib->dig_H4) << 20) - (((int32_t)calib->dig_H5) * v_x1_u32r)) + ((int32_t)16384)) >> 15) 
        * (((((((v_x1_u32r * ((int32_t)calib->dig_H6)) >> 10) * (((v_x1_u32r * ((int32_t)calib->dig_H3)) >> 11) + 
    ((int32_t)32768))) >> 10) + ((int32_t)2097152)) * ((int32_t)calib->dig_H2) + 8192) >> 14));

    v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) *
                ((int32_t)calib->dig_H1)) >> 4));

    v_x1_u32r = (v_x1_u32r < 0 ? 0 : v_x1_u32r);
    v_x1_u32r = (v_x1_u32r > 419430400 ? 419430400 : v_x1_u32r);

    return (uint32_t)(v_x1_u32r >> 12) / 1024.0f;
}

float bme280_pressure_butify(uint32_t raw_pressure) {
    return raw_pressure / 256.0f;
}

float bme280_humidity_butify(int64_t raw_humidity) {
    return raw_humidity / 1024.0f;
}

float bme280_temperature_butify(int32_t raw_temperature) {
    return raw_temperature / 100.0f;
}