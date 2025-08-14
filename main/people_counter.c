#include "stdlib.h"
#include <stdbool.h>
#include "people_counter.h"
#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ha/esp_zigbee_ha_standard.h"
#include "esp_zb_light.h"
#include "driver/i2c.h"

#define MEASURE_INTERVAL_MS 1000 // Intervalle de mesure en millisecondes
case_e last_zone = ZONE_0;

#define I2C_MASTER_SCL_IO 20      // GPIO pour SCL
#define I2C_MASTER_SDA_IO 19      // GPIO pour SDA
#define I2C_MASTER_NUM I2C_NUM_0  // Numéro du port I2C
#define I2C_MASTER_FREQ_HZ 400000 // Fréquence I2C
#define VL53L1X_ADDR 0x29         // Adresse I2C par défaut du VL53L0X

#define VL53L0X_REG_RESULT 0x14 // Registre de lecture des données
#define VL53L0X_REG_START 0x00  // Registre de démarrage du capteur

static const char *TAG = "VL53L1X";

vl53l1x_t *sensorInit()
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
        return NULL;
    }

    printf("Initialisation i2C ok\n");

    vl53l1x_setROISize(sensor, 8, 16); // FOV complet => 16*16

    vl53l1x_setROICenter(sensor, 199);
    printf("Configuration du capteur ok\n");
    return sensor; // Retourne le capteur initialisé
}

uint8_t people_counter(vl53l1x_t *sensor)
{
    bool detect1 = false;
    bool detect2 = false;
    static const char *TAG = "VL53L1X";
    uint8_t center[2] = {167, 231}; // Valeurs centre zone

    static uint8_t counter = 0; // Compteur de passage

    // Zone 1
    vl53l1x_setROICenter(sensor, center[0]);
    vTaskDelay(pdMS_TO_TICKS(100));
    uint16_t dist1 = vl53l1x_read(sensor, false);

    // Zone 2
    vl53l1x_setROICenter(sensor, center[1]);
    vTaskDelay(pdMS_TO_TICKS(100));
    uint16_t dist2 = vl53l1x_read(sensor, false);

    if (dist1 <= 400 && dist1 > 0)
    {
        detect1 = true;
    }
    else
    {
        detect1 = false;
    }
    if (dist2 <= 400 && dist2 > 0)
    {
        detect2 = true;
    }
    else
    {
        detect2 = false;
    }
    // printf("detect1: %d, detect2: %d\n", detect1, detect2);
    // Log pour debug
    ESP_LOGI(TAG, "dist1: %d, dist2: %d, last_zone: %d", dist1, dist2, last_zone);

    // Détection de passage
    if (detect1 && !detect2 && last_zone != ZONE_1 && last_zone == ZONE_0)
    {
        last_zone = ZONE_1;
    }
    else if (detect2 && !detect1 && last_zone == ZONE_1)
    {

        counter++;
        ESP_LOGI(TAG, "Passage zone1 -> zone2, compteur: %d", counter);
        last_zone = ZONE_0;
    }
    else if (detect2 && !detect1 && last_zone != ZONE_2 && last_zone == ZONE_0)
    {
        last_zone = ZONE_2;
    }
    else if (detect1 && !detect2 && last_zone == ZONE_2)
    {
        // printf("toto\n");
        counter = counter - 1;
        ESP_LOGI(TAG, "Passage zone2 -> zone1, compteur: %d", counter);

        last_zone = ZONE_0;
    }
    else if (!detect1 && !detect2 && last_zone != ZONE_0)
    {
        last_zone = ZONE_0; // Réinitialisation si aucune détection
    }

    if (counter == 255)
    {
        counter = 0; // Empêche le compteur de devenir négatif
    }
    printf("counter: %d\n", counter);
    return counter;
}

/**
 * @brief Fonction principale de l'application
 *
 * Cette fonction initialise le capteur VL53L1X, configure les zones de détection,
 * et gère la logique de détection des passages entre les zones.
 *
 */

void RTOS_task(void *pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(MEASURE_INTERVAL_MS); // 1 seconde
    vl53l1x_t *sensor = (vl53l1x_t *)pvParameters;                    // Récupération du capteur passé en paramètre
    uint8_t lastPeopleCount = 0;                                      // Dernier nombre de personnes comptées
    // uint16_t peopleCounter = 0;

    while (1)
    {

        printf("Tâche exécutée !\n");
        // peopleCounter = (peopleCounter + 1);
        // uint16_t test = peopleCounter *10;
        // if (peopleCounter == 255)
        // {
        // peopleCounter = 0; // Réinitialisation si le compteur dépasse 255
        // }
        uint16_t peopleCounter = people_counter(sensor) *100; // Appel de la fonction de comptage de personnes
        // peopleCounter++;
        // esp_zb_task(NULL); // Appel de la tâche Zigbee, si nécessaire
        if (peopleCounter != lastPeopleCount)
        {
            ESP_LOGI(TAG, "Nombre de personnes détectées: %d", peopleCounter);
            // reportAttribute(HA_ESP_LIGHT_ENDPOINT, ESP_ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT, ESP_ZB_ZCL_ATTR_TEMP_MEASUREMENT_VALUE_ID, &peopleCounter, 1);
            reportAttribute(HA_ESP_LIGHT_ENDPOINT, ESP_ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT, ESP_ZB_ZCL_ATTR_TEMP_MEASUREMENT_VALUE_ID, &peopleCounter, 2);

            lastPeopleCount = peopleCounter; // Mise à jour du dernier nombre de personnes comptées
        }
        else
        {
            printf("Aucun changement dans le nombre de personnes.\n");
            //lastPeopleCount = 1;
        }

        // vTaskDelayUntil(&xLastWakeTime, xFrequency);
        vTaskDelay(xFrequency); // Attendre l'intervalle défini avant la prochaine exécution
    }
}