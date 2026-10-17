#ifndef ADC_H
#define ADC_H
#include <stdbool.h>
#include <stdint.h>

struct adc_m {
    uint32_t raw;
    uint32_t percentage;
};

extern struct k_msgq adc_q;
void set_state_adc(bool pause);

#endif