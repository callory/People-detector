#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/FreeRTOSConfig.h"

#include "freertos/task.h"
#include "driver/i2c.h"


#include "esp_log.h"
#include "vl53l1x.h"


#define I2C_MASTER_SCL_IO 20    // GPIO pour SCL
#define I2C_MASTER_SDA_IO 19    // GPIO pour SDA
#define I2C_MASTER_NUM I2C_NUM_0 // Numéro du port I2C
#define I2C_MASTER_FREQ_HZ 400000 // Fréquence I2C
#define VL53L0X_ADDR 0x29         // Adresse I2C par défaut du VL53L0X

#define VL53L0X_REG_RESULT 0x14 // Registre de lecture des données
#define VL53L0X_REG_START 0x00  // Registre de démarrage du capteur

static const char *TAG = "VL53L1X";





void i2c_master_init();


void app_main() {
    vl53l1x_t *sensor = vl53l1x_config(0, I2C_MASTER_SCL_IO, I2C_MASTER_SDA_IO, -1, VL53L0X_ADDR, 0); // Configuration du capteur

    ESP_LOGI(TAG, "Démarrage du programme");
    vl53l1x_init(sensor); // Initialisation de la bibliothèque VL53L0X
    printf("Initialisation i2C ok\n");
    vl53l1x_setROISize(sensor, 16, 16); // FOV complet
    vl53l1x_setROICenter(sensor, 199); 
    printf("Configuration du capteur ok\n");
    vl53l1x_startContinuous(sensor, 0); // 0 pour un mode continu sans délai entre les mesures
    ESP_LOGI(TAG, "Mode continu démarré");

    while (1) {
        // uint16_t distance = vl53l0x_readRangeSingleMillimeters(sensor); // Lecture de la distance
        // ESP_LOGI(TAG, "Distance mesurée: %d mm", distance);
        // vTaskDelay(pdMS_TO_TICKS(500)); // Délai de 500 ms entre les lectures

         // Lecture de la distance en mode continu
         uint16_t distance = vl53l1x_read(sensor,0);
         if (vl53l1x_timeoutOccurred(sensor)) {
             ESP_LOGE(TAG, "Erreur : Timeout lors de la lecture de la distance");
         } else {
             //ESP_LOGI(TAG, "Distance mesurée : %d mm", distance);
         }
 
         vTaskDelay(pdMS_TO_TICKS(1000)); // Délai de 500 ms entre les lectures
         
    }

    vl53l1x_stopContinuous(sensor);
    vl53l1x_end(sensor);
    
    
  
}

























