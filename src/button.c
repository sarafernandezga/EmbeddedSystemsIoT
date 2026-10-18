#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <stdbool.h>
#include <stdint.h>
#include <errno.h>

#include "button.h"

/* Variables */
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(DT_ALIAS(userbt), gpios);

/* Button Callback */
static struct gpio_callback button_cb_data;

/* Initialize function*/
int init_button(void);

/* Button Event */
enum button_event {
    BUTTON_EVENT,
    LONG_PRESS_EXPIRED
};

/* Message Queues */
K_MSGQ_DEFINE_TYPE(button_evt_q, sizeof(enum button_event), 1);
K_MSGQ_DEFINE_TYPE(but_q, sizeof(enum button_mode), 1);

/* Long Press Timer */
static void long_pulse_handler(struct k_timer *timer);

K_TIMER_DEFINE(long_timer, long_pulse_handler, NULL);

/* Timer Callback */
static void long_pulse_handler(struct k_timer *timer)
{
    ARG_UNUSED(timer);

    /* Informamos que se ha ejecutado una pulsacion larga */
    enum button_event evt = LONG_PRESS_EXPIRED;
    k_msgq_put(&button_evt_q, &evt, K_NO_WAIT);
}

/* GPIO Interrupt Callback */
static void button_isr(const struct device *port,
                       struct gpio_callback *cb,
                       gpio_port_pins_t pins)
{   
    ARG_UNUSED(port);
    ARG_UNUSED(cb);
    ARG_UNUSED(pins);

    /* Informamos que se ha producido un cambio en el boton */
    enum button_event evt = BUTTON_EVENT;
    k_msgq_put(&button_evt_q, &evt, K_NO_WAIT);
    
}

/* Initialize button */
int init_button(void){

    if (!gpio_is_ready_dt(&button)) {
        printk("BUTTON: Error initializing GPIO\n");
        return -ENODEV;
    }

    /* Configure as input */
    int ret = gpio_pin_configure_dt(&button, GPIO_INPUT);

    if (ret != 0){
        printk("BUTTON: Error configuring GPIO\n");
        return ret;
    }

    /* Register Callback */
    gpio_init_callback(&button_cb_data, button_isr, BIT(button.pin));

    ret = gpio_add_callback(button.port, &button_cb_data);

    if (ret != 0){
        printk("BUTTON: Error adding callback\n");
        return ret;
    }

    /* Interrupt on both edges */
    ret = gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_BOTH);  //en vez de edge to activate both, asi devuelve o 0 o 1
    
    if (ret != 0){
        printk("BUTTON: Error configuring interruption\n");
        return ret;
    }

    return ret;
}

void my_thread_but(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    /* Initialize button */
    int ret = init_button();

    if (ret != 0){
        printk("BUTTON: Initialization failed\n");
        return;
    }

    /* Variables */
    enum button_event mevt;
    enum button_mode mbut;
    int pin_state;
    int long_set = 0;

    while (1) {
        // Wait for a button event
        ret = k_msgq_get(&button_evt_q, &mevt, K_FOREVER);

        if (ret == 0){
            switch (mevt) {
                case BUTTON_EVENT:                // Button event
                    /* Arrancar el antirrebote */
                    pin_state = gpio_pin_get_dt(&button);

                    if (pin_state < 0){
                        printk("BUTTON: Error reading GPIO\n");

                    } else if (pin_state == 1) {            // Sigue pulsado
                        /* Inicia el timer de pulsacion larga */
                        k_timer_start(&long_timer, K_SECONDS(1), K_NO_WAIT);

                    } else {                               // Soltado
                        /* Para el timer de pulsacion larga */
                        k_timer_stop(&long_timer);

                        /* Si no se ha producido pulsacion larga */
                        if (long_set == 0){
                            /* Se manda una pulsacion corta */
                            mbut = SHORT_PRESS;
                            k_msgq_put(&but_q, &mbut, K_NO_WAIT);
                        } 

                        /* Se reinicializa la variable de pulsacion larga */
                        long_set = 0;
                    } 
                    
                    break;

                case LONG_PRESS_EXPIRED:        // Long press
                        long_set = 1;
                        mbut = LONG_PRESS;
                        k_msgq_put(&but_q, &mbut, K_NO_WAIT);

                    break;
            }
        }
    }
}



K_THREAD_DEFINE(
    my_thread_but_id,  /* Identificador */
    1024,              /* Stack size */
    my_thread_but,  /* Funcion de entrada */
    NULL, NULL, NULL,  /* Parametros */
    4,                 /* Prioridad */
    0,                 /* Opciones */
    0                  /* Retardo de inicio */
);