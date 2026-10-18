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
K_MSGQ_DEFINE_TYPE(button_evt_q, enum button_event, 1);
K_MSGQ_DEFINE_TYPE(but_q, enum button_mode, 1);

/* Long Press Timer */
static void long_pulse_handler(struct k_timer *timer);

K_TIMER_DEFINE(long_timer, long_pulse_handler, NULL);

/* Timer Callback */
static void long_pulse_handler(struct k_timer *timer)
{
    ARG_UNUSED(timer);

    /* Sends an event when the long press timer expires */
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

   /* Notifies the thread when the button state changes */
    enum button_event evt = BUTTON_EVENT;
    k_msgq_put(&button_evt_q, &evt, K_NO_WAIT);
    
}

/* Initialize button */
int init_button(void){
    /* Check if the GPIO device is ready */
    if (!gpio_is_ready_dt(&button)) {
        printk("BUTTON: Error initializing GPIO\n");
        return -ENODEV;
    }

    /* Configure the button as input */
    int ret = gpio_pin_configure_dt(&button, GPIO_INPUT);

    if (ret != 0){
        printk("BUTTON: Error configuring GPIO\n");
        return ret;
    }

    /* Register the GPIO callback */
    gpio_init_callback(&button_cb_data, button_isr, BIT(button.pin));

    ret = gpio_add_callback(button.port, &button_cb_data);

    if (ret != 0){
        printk("BUTTON: Error adding callback\n");
        return ret;
    }

    /* Enable interrupts on both button edges */
    ret = gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_BOTH);  //en vez de edge to activate both, asi devuelve o 0 o 1
    
    if (ret != 0){
        printk("BUTTON: Error configuring interruption\n");
        return ret;
    }

    return ret;
}

/* Button Thread */
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
                case BUTTON_EVENT: 
                    /* Read the current button state */
                    pin_state = gpio_pin_get_dt(&button);

                    if (pin_state < 0){
                        printk("BUTTON: Error reading GPIO\n");

                    } else if (pin_state == 1) {
                         /* Start the long press timer when pressed */
                        k_timer_start(&long_timer, K_SECONDS(1), K_NO_WAIT);

                    } else { 
                        /* Stop the timer when the button is released */
                        k_timer_stop(&long_timer);

                        /* Send a short press if no long press occurred */
                        if (long_set == 0){
                            mbut = SHORT_PRESS;
                            k_msgq_put(&but_q, &mbut, K_NO_WAIT);
                        } 

                        /* Reset the long press flag */
                        long_set = 0;
                    } 
                    
                    break;

                case LONG_PRESS_EXPIRED: 
                    /* Register and send a long press event */
                    long_set = 1;
                    mbut = LONG_PRESS;
                    k_msgq_put(&but_q, &mbut, K_NO_WAIT);

                    break;
            }
        }
    }
}

/* Start Button Thread */
K_THREAD_DEFINE(
    my_thread_but_id,  /* Thread identifier */
    1024,              /* Stack size */
    my_thread_but,     /* Entry function */
    NULL, NULL, NULL,  /* Parameters */
    4,                 /* Priority */
    0,                 /* Options */
    0                  /* Start delay */
);