#include <zephyr/kernel.h>
#include <stdint.h>
#include <zephyr/sys/printk.h>

#include "rgb.h"
#include "adc.h"
#include "button.h"
#include "mode_manager.h"

#define RANGE_LOW  33
#define RANGE_HIGH 66

void my_thread_manager(void *p1, void *p2, void *p3)
{
    uint32_t led_mode;
    struct adc_m madc;
    uint16_t mbut;
    int ret;

    uint16_t event = NORMAL;

    while (1) {

        /* Esperar hasta 50 ms por un evento del botón */
        ret = k_msgq_get(&but_q, &mbut, K_MSEC(5));

        /* Procesar únicamente si hemos recibido un evento */
        if (ret == 0) {

            switch (event) {

            case NORMAL:
                if (mbut == SHORT_PRESS) {
                    event = BLUE;

                    led_mode = BLUE_LED;
                    k_msgq_put(&rgb_q, &led_mode, K_NO_WAIT);

                    printk("System BLUE\n");

                } else if (mbut == LONG_PRESS) {
                    event = OFF;

                    led_mode = OFF_LED;
                    k_msgq_put(&rgb_q, &led_mode, K_NO_WAIT);

                    printk("System OFF\n");
                }
                break;

            case BLUE:
                if (mbut == SHORT_PRESS) {
                    k_msgq_purge(&adc_q);
                    event = NORMAL;
                    printk("System NORMAL\n");

                } else if (mbut == LONG_PRESS) {
                    event = OFF;

                    led_mode = OFF_LED;
                    k_msgq_put(&rgb_q, &led_mode, K_NO_WAIT);

                    printk("System OFF\n");
                }
                break;

            case OFF:
                if (mbut == SHORT_PRESS) {
                    k_msgq_purge(&adc_q);
                    event = NORMAL;
                    printk("System NORMAL\n");
                }
                break;

            default:
                event = NORMAL;
                break;
            }
        }

        /* Leer el ADC únicamente en modo NORMAL */
        if (event == NORMAL) {

            ret = k_msgq_get(&adc_q, &madc, K_NO_WAIT);

            if (ret == 0) { 

                printk("LCD: percentage = %d %%\n",
                       madc.percentage);

                if (madc.percentage < RANGE_LOW) {
                    led_mode = RED_LED;
                } else if (madc.percentage > RANGE_HIGH) {
                    led_mode = GREEN_LED;
                } else {
                    led_mode = YELLOW_LED;
                }

                k_msgq_put(&rgb_q, &led_mode, K_NO_WAIT);
            }
        }
    }
}

K_THREAD_DEFINE(
    my_thread_manager_id,
    1024,
    my_thread_manager,
    NULL, NULL, NULL,
    4,
    0,
    0
);