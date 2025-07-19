#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/FreeRTOSConfig.h"

#include "freertos/task.h"
#include "driver/i2c.h"

#include "esp_log.h"
#include "vl53l1x.h"
#include "time.h"
#include "people_counter.h"
// #include "ha/esp_zigbee_ha_standard.h"
// #include "ha/zb_ha_device_config.h"
// #include "zcl/esp_zigbee_zcl_power_config.h"

#define I2C_MASTER_SCL_IO 20      // GPIO pour SCL
#define I2C_MASTER_SDA_IO 19      // GPIO pour SDA
#define I2C_MASTER_NUM I2C_NUM_0  // Numéro du port I2C
#define I2C_MASTER_FREQ_HZ 400000 // Fréquence I2C
#define VL53L1X_ADDR 0x29         // Adresse I2C par défaut du VL53L0X

#define VL53L0X_REG_RESULT 0x14 // Registre de lecture des données
#define VL53L0X_REG_START 0x00  // Registre de démarrage du capteur

static const char *TAG = "VL53L1X";

void app_main()
{

    vl53l1x_t *sensor = vl53l1x_config(I2C_NUM_0, I2C_MASTER_SCL_IO, I2C_MASTER_SDA_IO, -1, VL53L1X_ADDR, 1); // Configuration du capteur

    ESP_LOGI(TAG, "Démarrage du programme");

    const char *err = vl53l1x_init(sensor);

    vl53l1x_startContinuous(sensor, 100); // 0 pour un mode continu sans délai entre les mesures
    ESP_LOGI(TAG, "Mode continu démarré");

    vl53l1x_setDistanceMode(sensor, VL53L1X_Long); // Mode de distance

    if (err)
    {
        ESP_LOGE(TAG, "Erreur init VL53L1X: %s", err);
        return;
    }

    printf("Initialisation i2C ok\n");

    vl53l1x_setROISize(sensor, 8, 16); // FOV complet => 16*16

    vl53l1x_setROICenter(sensor, 199);
    printf("Configuration du capteur ok\n");
    xTaskCreate(RTOS_task,"people_counter_task",2048, sensor,1,NULL); // Création de la tâche pour le comptage de personnes

}
