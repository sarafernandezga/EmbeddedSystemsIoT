#include <zephyr/kernel.h>
#include <stdint.h>
#include <zephyr/sys/printk.h>

#include "rgb.h"
#include "adc.h"
#include "button.h"
#include "mode_manager.h"

#define RANGE_LOW  33
#define RANGE_HIGH 66

/* Manager Thread */
void my_thread_manager(void *p1, void *p2, void *p3)
{ 
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);   
    
    /* Variables */
    int ret;
    enum rgb_modes led_mode;
    struct adc_m madc;
    enum button_mode mbut;

    /* Initialize State Machine */
    enum manager_modes event = NORMAL;

    while (1) {

        /* Wait up to 50 ms for a button event */
        ret = k_msgq_get(&but_q, &mbut, K_MSEC(50));

        /* Process the event only if a message was received */
        if (ret == 0) {
            /* State Machine */
            switch (event) {
                case NORMAL:
                    if (mbut == SHORT_PRESS) {          // Short press -> BLUE
                        /* Suspend ADC measurements */
                        set_state_adc(true);

                        /* Turn on the blue LED */
                        led_mode = BLUE_LED;
                        k_msgq_put(&rgb_q, &led_mode, K_NO_WAIT);

                        /* Update and print the system state */
                        event = BLUE;
                        printk("System BLUE\n");

                    } else if (mbut == LONG_PRESS) {    // Long press -> OFF
                        /* Suspend ADC measurements */
                        set_state_adc(true);

                        /* Turn off the LED */
                        led_mode = OFF_LED;
                        k_msgq_put(&rgb_q, &led_mode, K_NO_WAIT);

                        /* Update and print the system state */
                        event = OFF;
                        printk("System OFF\n");
                    }
                    break;

                case BLUE:
                    if (mbut == SHORT_PRESS) {         // Short press -> NORMAL
                        /* Clear pending ADC measurements and resume the ADC */
                        k_msgq_purge(&adc_q);
                        set_state_adc(false);

                        /* Update and print the system state */
                        event = NORMAL;
                        printk("System NORMAL\n");

                    } else if (mbut == LONG_PRESS) {    // Long press -> OFF
                    /* Suspend ADC measurements */
                    set_state_adc(true);

                    /* Turn off the LED */
                    led_mode = OFF_LED;
                    k_msgq_put(&rgb_q, &led_mode, K_NO_WAIT);

                    /* Update and print the system state */
                    event = OFF;
                    printk("System OFF\n");
                    }
                    break;

                case OFF:
                    if (mbut == SHORT_PRESS) {  // Short press -> NORMAL
                        /* Clear pending ADC measurements and resume the ADC */
                        k_msgq_purge(&adc_q);
                        set_state_adc(false);

                        /* Update and print the system state */
                        event = NORMAL;
                        printk("System NORMAL\n");
                    }

                    // Long press -> Remain in OFF

                    break;

                default:
                    event = NORMAL;
                    break;
            }
        }

        /* Read ADC measurements only in NORMAL mode */
        if (event == NORMAL) {

            ret = k_msgq_get(&adc_q, &madc, K_NO_WAIT);

            /* Process the measurement if a valid message was received */
            if (ret == 0) { 
                /* Print the measured percentage */
                printk("LCD: percentage = %d%%\n", madc.percentage);

                /* Select the LED color according to the light level */
                if (madc.percentage < RANGE_LOW) {
                    led_mode = RED_LED;
                } else if (madc.percentage > RANGE_HIGH) {
                    led_mode = GREEN_LED;
                } else {
                    led_mode = YELLOW_LED;
                }

                /* Send the selected LED mode to the RGB thread */
                k_msgq_put(&rgb_q, &led_mode, K_NO_WAIT);
            }
        }
    }
}

/* Start Manager Thread */
K_THREAD_DEFINE(
    my_thread_manager_id,  /* Thread identifier */
    1024,                  /* Stack size */
    my_thread_manager,     /* Entry function */
    NULL, NULL, NULL,      /* Parameters */
    4,                     /* Priority */
    0,                     /* Options */
    0                      /* Start delay */
);