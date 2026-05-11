const char *TAG = "BME280_FINAL";

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/i2c_master.h"
#include "driver/i2c_types.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include "ssd1306.h"
#include "bme280.h"
#include "pins.h"

#include "spi.h"

static bme280_calib_t cal;
static spi_device_handle_t spi_handle;
static SSD1306_t display_device;

static uint32_t pressure;
static int64_t humidity;
static int32_t temperature;

// I2C display
i2c_master_dev_handle_t* initAndSetupDisplay(i2c_master_bus_handle_t *bus_handler) {
    i2c_device_config_t rtc_dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = 0x68,
        .scl_speed_hz = 100000,
    };

    i2c_master_dev_handle_t *master_rtc_handle = malloc(sizeof(i2c_master_dev_handle_t));

    ESP_ERROR_CHECK(i2c_master_bus_add_device(*bus_handler, &rtc_dev_cfg, master_rtc_handle));

    return master_rtc_handle;
}

void init_and_setup_ssd1306() {
    ssd1306_init(&display_device, 128, 64);

	vTaskDelay(pdMS_TO_TICKS(500));

	ssd1306_clear_screen(&display_device, false);
	ssd1306_contrast(&display_device, 0xff);
}


void renderDisplayIterationHandler() {
    char strLen = 12;

    char tempString[strLen];
    char tempString_2[strLen];
    char tempString_3[strLen];

    snprintf(tempString, sizeof(tempString), "T: %.2f", bme280_temperature_butify(temperature));
    snprintf(tempString_2, sizeof(tempString_2), "H: %.2f", bme280_humidity_butify(humidity));
    snprintf(tempString_3, sizeof(tempString_3), "P: %.2f", bme280_pressure_butify(pressure));

    ssd1306_display_text(&display_device, 1, tempString, strLen, 0); 
    ssd1306_display_text(&display_device, 2, tempString_2, strLen, 0); 
    ssd1306_display_text(&display_device, 3, tempString_3, strLen, 0); 
}

void setup() {
     gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << MY_OUTPUT_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    
    gpio_config(&io_conf);

    gpio_set_level(MY_OUTPUT_PIN, 1);

	i2c_master_init(&display_device, I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO, I2C_MASTER_RST_PIN);
    
    init_and_setup_ssd1306();

    spi_bus_config_t buscfg = {
        .miso_io_num = PIN_NUM_MISO, 
        .mosi_io_num = PIN_NUM_MOSI,
        .sclk_io_num = PIN_NUM_CLK, 
        .quadwp_io_num = -1, 
        .quadhd_io_num = -1
    };
    spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 1000000, 
        .mode = 0,
        .spics_io_num = PIN_NUM_CS, 
        .queue_size = 7
    };
    spi_bus_add_device(SPI2_HOST, &devcfg, &spi_handle);

    spi_write_reg(spi_handle, 0xE0, 0xB6); // Reset

    vTaskDelay(pdMS_TO_TICKS(100));
    
    spi_write_reg(spi_handle, 0xF4, 0x27); // Normal mode

    BME280_setup_compensate(spi_handle, &cal);
}


void app_main(void) {
    setup();

    while (1) {
        uint8_t raw[8];
        spi_read_regs(spi_handle, 0xF7, raw, 8);
        
        uint32_t adc_press = ((uint32_t)raw[0] << 12) | ((uint32_t)raw[1] << 4) | ((uint32_t)raw[2] >> 4);
        uint32_t adc_temper = ((uint32_t)raw[3] << 12) | ((uint32_t)raw[4] << 4) | ((uint32_t)raw[5] >> 4);
        uint32_t adc_humidity = ((uint32_t)raw[6] << 8) | (uint32_t)raw[7];

        if (adc_temper == 0x80000) {
            printf("Ошибка: датчик еще не готов...\n");
        } else {
            temperature = BME280_compensate_T_int32(&cal, adc_temper);
            humidity = bme280_compensate_H_int32(&cal, adc_humidity);
            pressure = BME280_compensate_P_int64(&cal, adc_press);
        }

        renderDisplayIterationHandler();

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}