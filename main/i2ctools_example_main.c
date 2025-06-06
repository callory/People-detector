#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/FreeRTOSConfig.h"

#include "freertos/task.h"
#include "driver/i2c.h"

#include "esp_log.h"
#include "vl53l1x.h"

#define I2C_MASTER_SCL_IO 20      // GPIO pour SCL
#define I2C_MASTER_SDA_IO 19      // GPIO pour SDA
#define I2C_MASTER_NUM I2C_NUM_0  // Numéro du port I2C
#define I2C_MASTER_FREQ_HZ 400000 // Fréquence I2C
#define VL53L1X_ADDR 0x29         // Adresse I2C par défaut du VL53L0X

#define VL53L0X_REG_RESULT 0x14 // Registre de lecture des données
#define VL53L0X_REG_START 0x00  // Registre de démarrage du capteur

static const char *TAG = "VL53L1X";

void i2c_master_init()
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,

        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    ESP_ERROR_CHECK(i2c_param_config(I2C_MASTER_NUM, &conf));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0));
}
void i2c_scan()
{
    printf("Scan I2C en cours...\n");
    for (uint8_t addr = 1; addr < 127; addr++)
    {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
        i2c_master_stop(cmd);
        esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 50 / portTICK_PERIOD_MS);
        i2c_cmd_link_delete(cmd);
        if (ret == ESP_OK)
        {
            printf(" - Trouvé à l'adresse 0x%02X\n", addr);
        }
    }
    printf("Scan I2C terminé.\n");
}
void app_main()
{
    // i2c_master_init();

    // i2c_scan(); // Ajoute ceci ici                                                                 // Attente de 3 secondes pour s'assurer que l'I2C est prêt
    vl53l1x_t *sensor = vl53l1x_config(I2C_NUM_0, I2C_MASTER_SCL_IO, I2C_MASTER_SDA_IO, -1, VL53L1X_ADDR, 1); // Configuration du capteur

    ESP_LOGI(TAG, "Démarrage du programme");

    const char *err = vl53l1x_init(sensor);

    vl53l1x_startContinuous(sensor, 100); // 0 pour un mode continu sans délai entre les mesures
    ESP_LOGI(TAG, "Mode continu démarré");

    vl53l1x_setDistanceMode(sensor, VL53L1X_Medium); // Mode de distance
  
    if (err)
    {
        ESP_LOGE(TAG, "Erreur init VL53L1X: %s", err);
        return;
    }
    printf("Initialisation i2C ok\n");
    vl53l1x_setROISize(sensor, 16, 16); // FOV complet
    vl53l1x_setROICenter(sensor, 199);
    printf("Configuration du capteur ok\n");

    while (1)
    {
        //     vl53l1x_readSingle(sensor, 1);
        // ESP_LOGI(TAG, "Mode single démarré");
        // uint16_t distance = vl53l0x_readRangeSingleMillimeters(sensor); // Lecture de la distance
        // ESP_LOGI(TAG, "Distance mesurée: %d mm", distance);
        // vTaskDelay(pdMS_TO_TICKS(500)); // Délai de 500 ms entre les lectures

        // Lecture de la distance en mode continu
        uint16_t dist = vl53l1x_read(sensor, false);
        ESP_LOGI(TAG, "Lecture Distance  avant if: %d", dist);
        if (vl53l1x_dataReady(sensor))
        {
            uint16_t dist = vl53l1x_read(sensor, false);
            ESP_LOGI(TAG, "Distance : %d", dist);
        }
        else
        {
            ESP_LOGW(TAG, "Donnée pas prête");
        }

        vTaskDelay(pdMS_TO_TICKS(1000)); // Délai de 500 ms entre les lectures
    }

    vl53l1x_stopContinuous(sensor);
    vl53l1x_end(sensor);
}
