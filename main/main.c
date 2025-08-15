#include "esp_zb_light.h"

void app_main()
{

    xTaskCreate(esp_zb_task, "Zigbee_main", 4096, NULL, 5, NULL);
}
