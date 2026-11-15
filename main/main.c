#include "esp_zb_light.h" *
#include "esp_pm.h"


void app_main()
{
    // Permet d'activer le mode de sommeil léger et de configurer la fréquence du processeur
    esp_pm_config_t pm_config = {
        .max_freq_mhz = CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ,
        .min_freq_mhz = CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ,
        .light_sleep_enable = true, // Activer le mode de sommeil léger
    };
    ESP_ERROR_CHECK(esp_pm_configure(&pm_config));
    

    xTaskCreate(esp_zb_task, "Zigbee_main", 4096, NULL, 5, NULL);
}
