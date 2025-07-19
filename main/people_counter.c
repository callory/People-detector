#include "stdlib.h"
#include <stdbool.h>
#include "people_counter.h"
#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define MEASURE_INTERVAL_MS 1000 // Intervalle de mesure en millisecondes
case_e last_zone = ZONE_0;

void people_counter(vl53l1x_t *sensor)
{
    bool detect1 = false;
    bool detect2 = false;
    static const char *TAG = "VL53L1X";
    uint8_t center[2] = {167, 231}; // Valeurs centre zone

    static uint8_t counter = 0; // Compteur de passag
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
    printf("detect1: %d, detect2: %d\n", detect1, detect2);
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
        printf("toto\n");
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
    vl53l1x_t *sensor = (vl53l1x_t *)pvParameters; // Récupération du capteur passé en paramètre
    while (1)
    {
        // Ton code à exécuter toutes les secondes
        printf("Tâche exécutée !\n");
        people_counter(sensor); // Appel de la fonction de comptage de personnes


        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}