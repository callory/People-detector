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

typedef enum
{
    ZONE1_FIRST,
    ZONE2_FIRST,
} case_e;

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

    vl53l1x_setDistanceMode(sensor, VL53L1X_Long); // Mode de distance

    if (err)
    {
        ESP_LOGE(TAG, "Erreur init VL53L1X: %s", err);
        return;
    }
    uint8_t center[2] = {167, 231}; // Valeurs centre zone
    uint8_t zone = 0;
    // uint8_t counter = 0;
    bool zone1 = false;
    bool zone2 = false;
    printf("Initialisation i2C ok\n");
    // vl53l1x_setROISize(sensor, 16, 16); // FOV complet
    vl53l1x_setROISize(sensor, 8, 16); // FOV complet

    vl53l1x_setROICenter(sensor, 199);
    printf("Configuration du capteur ok\n");
    case_e my_case = 0;
    int counter = 0;
    int last_zone = 0; // 0: aucune, 1: zone1, 2: zone2
    bool detect1 = false;
    bool detect2 = false;
    while (1)
    {
        // Zone 1
        vl53l1x_setROICenter(sensor, center[0]);
        vTaskDelay(pdMS_TO_TICKS(100));
        uint16_t dist1 = vl53l1x_read(sensor, false);

        // Zone 2
        vl53l1x_setROICenter(sensor, center[1]);
        vTaskDelay(pdMS_TO_TICKS(100));
        uint16_t dist2 = vl53l1x_read(sensor, false);

        // Seuil de détection (ajuste si besoin)
        // bool detect1 = dist1 <= 200 && dist1 > 0;
        // bool detect2 = dist2 <= 200 && dist2 > 0;

        if (dist1 <= 200 && dist1 > 0)
        {
            detect1 = true;
        }
        else
        {
            detect1 = false;
        }
        if (dist2 <= 200 && dist2 > 0)
        {
            detect2 = true;
        }
        else
        {
            detect2 = false;
        }
        printf("detect1: %d, detect2: %d\n", detect1, detect2);
        // Log pour debug
        ESP_LOGI(TAG, "dist1: %d, dist2: %d, last_zone: %d", dist1, dist2, last_zone);

        // Détection de passage
        if (detect1 && !detect2 && last_zone != 1 && last_zone == 0)
        {
            last_zone = 1;
        }
        else if (detect2 && !detect1 && last_zone == 1)
        {
            counter++;
            ESP_LOGI(TAG, "Passage zone1 -> zone2, compteur: %d", counter);
            last_zone = 0;
        }
        else if (detect2 && !detect1 && last_zone != 2 && last_zone == 0)
        {
            last_zone = 2;
        }
        else if (detect1 && !detect2 && last_zone == 2)
        {
            printf("toto\n");
            counter = counter - 1;
            ESP_LOGI(TAG, "Passage zone2 -> zone1, compteur: %d", counter);
            last_zone = 0;
        }
        printf("counter: %d\n", counter);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    vl53l1x_stopContinuous(sensor);
    vl53l1x_end(sensor);
}
