#include "esp_zb_light.h" *
#include "esp_pm.h"

static void pm_debug_task(void *arg)
{
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(10000));
        esp_pm_dump_locks(stdout);
    }
}

void app_main()
{
    // Permet d'activer le mode de sommeil léger et de configurer la fréquence du processeur
    esp_pm_config_t pm_config = {
        .max_freq_mhz = CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ,
        .min_freq_mhz = CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ,
        .light_sleep_enable = true,
    };
    ESP_ERROR_CHECK(esp_pm_configure(&pm_config));
    // xTaskCreate(pm_debug_task, "pm_debug", 3072, NULL, 1, NULL);

    xTaskCreate(esp_zb_task, "Zigbee_main", 4096, NULL, 5, NULL);
}
