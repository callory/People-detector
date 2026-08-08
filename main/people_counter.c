#include "stdlib.h"
#include <stdbool.h>
#include "people_counter.h"
#include <stdio.h>
#include <string.h>
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
#define XSHUT_PIN 7               // GPIO pour le pin XSHUT
#define I2C_MASTER_NUM I2C_NUM_0  // Numéro du port I2C
#define I2C_MASTER_FREQ_HZ 400000 // Fréquence I2C
#define VL53L1X_ADDR 0x29         // Adresse I2C par défaut du VL53L0X

#define VL53L0X_REG_RESULT 0x14 // Registre de lecture des données
#define VL53L0X_REG_START 0x00  // Registre de démarrage du capteur

static const char *TAG = "VL53L1X";

void i2c_scan(void)
{
    printf("Scan I2C en cours...\n");

    for (uint8_t addr = 0x08; addr < 0x78; addr++)
    {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
        i2c_master_stop(cmd);

        esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 50 / portTICK_PERIOD_MS);
        // printf("I2C scan result: %d\n", ret);
        i2c_cmd_link_delete(cmd);

        if (ret == ESP_OK)
        {
            printf("   → Périphérique trouvé à l'adresse 0x%02X\n", addr);
            break;
        }
    }

    printf("Scan terminé.\n");
}

vl53l1x_t *sensorInit()
{
    ESP_LOGI(TAG, "=== INITIALISATION CAPTEUR VL53L1X ===");

    // Configurer le capteur (installe aussi le driver I2C)
    ESP_LOGI(TAG, "Configuration I2C (port %d, SCL=%d, SDA=%d, adresse=0x%02X)",
             I2C_MASTER_NUM, I2C_MASTER_SCL_IO, I2C_MASTER_SDA_IO, VL53L1X_ADDR);
    vl53l1x_t *sensor = vl53l1x_config(I2C_MASTER_NUM, I2C_MASTER_SCL_IO, I2C_MASTER_SDA_IO, XSHUT_PIN, VL53L1X_ADDR, 0);

    if (sensor == NULL)
    {
        ESP_LOGE(TAG, "ERREUR: vl53l1x_config() a retourné NULL");
        ESP_LOGE(TAG, "Vérifiez: GPIO valides, alimentation 3.3V, broches SCL/SDA connectées");
        return NULL;
    }

    // Initialiser le capteur
    ESP_LOGI(TAG, "Initialisation du VL53L1X...");
    const char *err = vl53l1x_init(sensor);

    if (err)
    {
        ESP_LOGE(TAG, "=== ERREUR INITIALISATION CAPTEUR ===");
        ESP_LOGE(TAG, "Code erreur: %s", err);

        // Diagnostic détaillé basé sur le code d'erreur
        if (strcmp(err, "Not VL53L1X") == 0)
        {
            ESP_LOGE(TAG, "Le modèle ID reçu n'est pas 0xEACC (valeur attendue pour VL53L1X)");
            ESP_LOGE(TAG, "DIAGNOSTIC: Le capteur ne répond pas correctement sur I2C");
            ESP_LOGE(TAG, "Vérifiez:");
            ESP_LOGE(TAG, "  • Alimentation 3.3V stabile au capteur");
            ESP_LOGE(TAG, "  • Broches SCL (GPIO20) et SDA (GPIO19) bien connectées");
            ESP_LOGE(TAG, "  • Pull-up résistances (~4.7kΩ) sur SCL et SDA");
            ESP_LOGE(TAG, "  • Capteur VL53L1X (et non VL53L0X ou autre modèle)");
        }
        else if (strcmp(err, "Timeout") == 0)
        {
            ESP_LOGE(TAG, "Timeout en attente du démarrage du capteur");
            ESP_LOGE(TAG, "Le capteur ne répond pas aux commandes I2C");
        }

        free(sensor); // Libérer la mémoire allouée
        return NULL;
    }
    else
    {
        // vl53l1x_startContinuous(sensor, 25); // 0 pour un mode continu sans délai entre les mesures
        ESP_LOGI(TAG, "Mode continu démarré");

        vl53l1x_setDistanceMode(sensor, VL53L1X_Short); // Mode de distance
        printf("Initialisation i2C ok\n");
        // j'avais mis 8*16 pourquoi je ne sais pas
        // vl53l1x_setROISize(sensor, 8, 16); // FOV partiel => 8*16
        vl53l1x_setROISize(sensor, 8, 8); // FOV complet => 16*16

        vl53l1x_setROICenter(sensor, 199);

        // Attendre que la première mesure soit disponible (~200ms pour mode Long)
        // vTaskDelay(pdMS_TO_TICKS(200));

        printf("Configuration du capteur ok\n");
        ESP_LOGI(TAG, "✓ Capteur VL53L1X initialisé avec succès!\n");
        return sensor; // Retourne le capteur initialisé
    }
}

uint8_t people_counter(vl53l1x_t *sensor)
{
    bool detect1 = false;
    bool detect2 = false;
    static const char *TAG = "VL53L1X";
    uint8_t center[2] = {167, 223}; // Valeurs cened
    static uint8_t counter = 0;     // Compteur de passage

    // Zone 1 - Lecture bloquante pour garantir donnée fraîche
    vl53l1x_setROICenter(sensor, center[0]);
    // vTaskDelay(pdMS_TO_TICKS(100)); // Attendre que la mesure soit prête
    // uint16_t dist1 = vl53l1x_read(sensor, true); // true = BLOQUANT (attendre la donnée)
    uint16_t dist1 = vl53l1x_readSingle(sensor, true);
    // Zone 2 - Lecture bloquante pour garantir donnée fraîche
    vl53l1x_setROICenter(sensor, center[1]);
    // vTaskDelay(pdMS_TO_TICKS(100)); // Attendre que la mesure soit prête
    // uint16_t dist2 = vl53l1x_read(sensor, true); // true = BLOQUANT (attendre la donnée)
    uint16_t dist2 = vl53l1x_readSingle(sensor, true);
    if (dist1 <= 1200 && dist1 > 0)
    {
        detect1 = true;
    }
    else
    {
        detect1 = false;
    }
    if (dist2 <= 1200 && dist2 > 0)
    {
        detect2 = true;
    }
    else
    {
        detect2 = false;
    }

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

void RTOS_task(void *pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(MEASURE_INTERVAL_MS); // 1 seconde
    vl53l1x_t *sensor = (vl53l1x_t *)pvParameters;                    // Récupération du capteur passé en paramètre
    uint8_t lastPeopleCount = 0;                                      // Dernier nombre de personnes comptées

    while (1)
    {
        // printf("Tâche exécutée !\n");

        uint16_t peopleCounter = people_counter(sensor) * 100; // Appel de la fonction de comptage de personnes

        if (peopleCounter != lastPeopleCount)
        {
            ESP_LOGI(TAG, "Nombre de personnes détectées: %d", peopleCounter);
            reportAttribute(HA_ESP_LIGHT_ENDPOINT, ESP_ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT, ESP_ZB_ZCL_ATTR_TEMP_MEASUREMENT_VALUE_ID, &peopleCounter, 2);

            lastPeopleCount = peopleCounter; // Mise à jour du dernier nombre de personnes comptées
        }
        else
        {
            // printf("Aucun changement dans le nombre de personnes.\n");
            ESP_LOGI(TAG, "Aucun changement dans le nombre de personnes");
        }
        vTaskDelayUntil(&xLastWakeTime, xFrequency); // Attente jusqu'à la prochaine exécution
    }
}