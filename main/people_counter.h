#ifndef PEOPELE_COUNTER_H
#define PEOPELE_COUNTER_H

#include "vl53l1x.h"

typedef enum
{
    ZONE_0 = 0,
    ZONE_1,
    ZONE_2
}case_e;
void people_counter(vl53l1x_t *sensor);
#endif // PEOPELE_COUNTER_H