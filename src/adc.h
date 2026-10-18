#ifndef ADC_H
#define ADC_H

#include <stdbool.h>
#include <stdint.h>
#include <zephyr/kernel.h>

/* Struct ADC Message */
struct adc_m {
    uint32_t raw;
    uint32_t percentage;
};

/* Message Queue ID */
extern struct k_msgq adc_q;

/* Semaphore ID */
extern struct k_sem sem_adc_gate;

/* ADC Pause Control */
void set_state_adc(bool pause);

#endif