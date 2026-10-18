#include <zephyr/kernel.h>
#include <stdint.h>
#include <zephyr/sys/printk.h>

#include "rgb.h"
#include "adc.h"
#include "button.h"
#include "mode_manager.h"

/* Brightness range */
#define RANGE_LOW  33
#define RANGE_HIGH 66

/* Timer */
/* Manejador del timer que desbloquea el ADC */
static void adc_timer_handler(struct k_timer *timer)
{
    ARG_UNUSED(timer);
    k_sem_give(&sem_adc_gate);
}

K_TIMER_DEFINE(adc_timer, adc_timer_handler, NULL);

/* Manager Thread */
void my_thread_manager(void *p1, void *p2, void *p3)
{ 
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);   
    
    /* Variables */
    uint32_t led_mode;
    struct adc_m madc;
    uint16_t mbut;
    int ret;

    /* Initialize State Machine */
    uint16_t event = NORMAL;
    k_timer_start(&adc_timer, K_SECONDS(2), K_SECONDS(2));   // Activa directamente el timer periodico
    k_sem_give(&sem_adc_gate);                               // Solicita ya una primera medida del ADC

    while (1) {

        /* Esperar hasta 50 ms por un evento del botón */
        ret = k_msgq_get(&but_q, &mbut, K_MSEC(50));

        /* Si se ha realizado alguna pulsacion */
        if (ret == 0) {
            /* Manejador de cambio de estados */
            switch (event) {

            case NORMAL:
                if (mbut == SHORT_PRESS) {  // Pulsacion corta -> BLUE
                    /* Dejamos de activar el hilo ADC */
                    k_timer_stop(&adc_timer);
                    k_sem_reset(&sem_adc_gate);

                    /* Encendemos el led azul */
                    led_mode = BLUE_LED;
                    k_msgq_put(&rgb_q, &led_mode, K_NO_WAIT);

                    /* Escribimos por pantalla el estado */
                    printk("System BLUE\n");
                    event = BLUE;

                } else if (mbut == LONG_PRESS) {  // Pulsacion larga -> OFF
                    /* Dejamos de activar el hilo ADC */
                    k_timer_stop(&adc_timer);
                    k_sem_reset(&sem_adc_gate);

                    /* Apagamos el led */
                    led_mode = OFF_LED;
                    k_msgq_put(&rgb_q, &led_mode, K_NO_WAIT);

                    /* Escribimos por pantalla el estado */
                    printk("System OFF\n");
                    event = OFF;

                }
                break;

            case BLUE:
                if (mbut == SHORT_PRESS) {  // Pulsacion corta -> NORMAL
                    
                    /* Activamos el hilo del ADC y el temporizador periodico */
                    k_sem_give(&sem_adc_gate);
                    k_msgq_purge(&adc_q);
                    k_timer_start(&adc_timer, K_SECONDS(2), K_SECONDS(2));

                    /* Escribimos por pantalla el estado */
                    printk("System NORMAL\n");
                    event = NORMAL;

                } else if (mbut == LONG_PRESS) {  // Pulsacion larga -> OFF
                    
                    /* Apagamos el led */
                    led_mode = OFF_LED;
                    k_msgq_put(&rgb_q, &led_mode, K_NO_WAIT);

                    /* Escribimos por pantalla el estado */
                    printk("System OFF\n");
                    event = OFF;
                }
                break;

            case OFF:
                if (mbut == SHORT_PRESS) {  // Pulsacion corta -> NORMAL
                    /* Activamos el hilo del ADC y el temporizador periodico */
                    k_sem_give(&sem_adc_gate);
                    k_msgq_purge(&adc_q);
                    k_timer_start(&adc_timer, K_SECONDS(2), K_SECONDS(2));

                    /* Escribimos por pantalla el estado */
                    printk("System NORMAL\n");
                    event = NORMAL;
                }

                // Pulsacion larga -> Sigue en OFF
                break;

            default:
                event = NORMAL;
                break;
            }
        }

        /* Leer el ADC únicamente en modo NORMAL */
        if (event == NORMAL) {
            /* Lee los datos leidos del ADC */
            ret = k_msgq_get(&adc_q, &madc, K_NO_WAIT);

            /* Si recibe un mensaje valido se ejecuta*/
            if (ret == 0) { 
                /* Muestra el valor del porcentaje por pantalla */
                printk("LCD: percentage = %d %%\n", madc.percentage);

                /* Cambia al color correspondiente del led */
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

/* Thread defining */
K_THREAD_DEFINE(
    my_thread_manager_id,
    1024,
    my_thread_manager,
    NULL, NULL, NULL,
    4,
    0,
    0
);