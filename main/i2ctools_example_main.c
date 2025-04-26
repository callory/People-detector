#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "vl53l0x.h"

#define I2C_MASTER_SCL_IO 20    // GPIO pour SCL
#define I2C_MASTER_SDA_IO 19    // GPIO pour SDA
#define I2C_MASTER_NUM I2C_NUM_0 // Numéro du port I2C
#define I2C_MASTER_FREQ_HZ 400000 // Fréquence I2C
#define VL53L0X_ADDR 0x29         // Adresse I2C par défaut du VL53L0X

#define VL53L0X_REG_RESULT 0x14 // Registre de lecture des données
#define VL53L0X_REG_START 0x00  // Registre de démarrage du capteur

static const char *TAG = "VL53L0X";



void i2c_master_init();


void app_main() {
    vl53l0x_t *sensor = vl53l0x_config(0, I2C_MASTER_SCL_IO, I2C_MASTER_SDA_IO, -1, VL53L0X_ADDR, 0); // Configuration du capteur

    ESP_LOGI(TAG, "Démarrage du programme");
    vl53l0x_init(sensor); // Initialisation de la bibliothèque VL53L0X
    
    printf("Initialisation i2C ok\n");
    while (1) {
        uint16_t distance = vl53l0x_readRangeSingleMillimeters(sensor); // Lecture de la distance
        ESP_LOGI(TAG, "Distance mesurée: %d mm", distance);
        vTaskDelay(pdMS_TO_TICKS(500)); // Délai de 500 ms entre les lectures
    }
    
    
  
}




































// esp_err_t vl53l0x_init() {
//     uint8_t start = 0x01; // Valeur pour démarrer le capteur
//     i2c_cmd_handle_t cmd = i2c_cmd_link_create();
//     i2c_master_start(cmd);
//     i2c_master_write_byte(cmd, (VL53L0X_ADDR << 1) | I2C_MASTER_WRITE, true);
//     i2c_master_write_byte(cmd, VL53L0X_REG_START, true);
//     i2c_master_write_byte(cmd, start, true);
//     i2c_master_stop(cmd);
//     esp_err_t err = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_PERIOD_MS);
//     i2c_cmd_link_delete(cmd);
//     return err;
// }

// esp_err_t vl53l0x_read_range(uint16_t *range) {
//     uint8_t data[2];
//     i2c_cmd_handle_t cmd = i2c_cmd_link_create();
//     i2c_master_start(cmd);
//     i2c_master_write_byte(cmd, (VL53L0X_ADDR << 1) | I2C_MASTER_WRITE, true);
//     i2c_master_write_byte(cmd, VL53L0X_REG_RESULT, true);
//     i2c_master_start(cmd);
//     i2c_master_write_byte(cmd, (VL53L0X_ADDR << 1) | I2C_MASTER_READ, true);
//     i2c_master_read(cmd, data, 2, I2C_MASTER_LAST_NACK);
//     i2c_master_stop(cmd);
//     esp_err_t err = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_PERIOD_MS);
//     i2c_cmd_link_delete(cmd);
//     if (err == ESP_OK) {
//         *range = (data[0] << 8) | data[1];
//     }
//     return err;
// }

// void lidar_task(void *pvParameter) {
//     uint16_t distance;
//     while (1) {
//         if (vl53l0x_read_range(&distance) == ESP_OK) {
//             ESP_LOGI(TAG, "Distance mesurée: %d mm", distance);
//         } else {
//             ESP_LOGE(TAG, "Erreur de lecture du capteur");
//         }
//         vTaskDelay(pdMS_TO_TICKS(500));
//     }
// }

// void app_main() {
//     ESP_LOGI(TAG, "Démarrage du programme");
//     i2c_master_init();
//     if (vl53l0x_init() == ESP_OK) {
//         ESP_LOGI(TAG, "Capteur VL53L0X initialisé avec succès");
//     } else {
//         ESP_LOGE(TAG, "Échec de l'initialisation du VL53L0X");
//     }
//     xTaskCreate(&lidar_task, "lidar_task", 4096, NULL, 5, NULL);
// }
