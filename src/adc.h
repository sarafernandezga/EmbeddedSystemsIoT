#ifndef ADC_H
#define ADC_H

#include <stdint.h>

struct adc_m {
    uint32_t raw;
    uint32_t percentage;
};

extern struct k_msgq adc_q;
#endif