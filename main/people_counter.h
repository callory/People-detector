#ifndef PEOPLE_COUNTER_H
#define PEOPLE_COUNTER_H

#include "vl53l1x.h"

typedef enum
{
    ZONE_0 = 0,
    ZONE_1,
    ZONE_2
}case_e;
uint8_t people_counter(vl53l1x_t *sensor);
void RTOS_task(void *pvParameters);
vl53l1x_t * sensorInit();
#endif // PEOPLE_COUNTER_H