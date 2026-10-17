#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <stdbool.h>
#include <stdint.h>
#include "rgb.h"
#include "button.h"

/* Variables */
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(DT_ALIAS(userbt), gpios);

/* Button Callback */
static struct gpio_callback button_cb_data;

/* Timers */
struct button_state {
    bool long_press_detected;
};

static struct button_state btn_state = {
    .long_press_detected = false
};

static void debounce_handler(struct k_timer *timer);
static void long_pulse_handler(struct k_timer *timer);

K_TIMER_DEFINE(debounce_timer, debounce_handler, NULL);
K_TIMER_DEFINE(long_pulse_timer, long_pulse_handler, NULL);

/* Message */
K_MSGQ_DEFINE_TYPE(but_q, sizeof(uint16_t), 1);

static void debounce_handler(struct k_timer *timer)
{
    ARG_UNUSED(timer);
    int pin_state = gpio_pin_get_dt(&button); // recogemos si está activo o no
    struct button_state *state = k_timer_user_data_get(timer);


     if (pin_state < 0){  // Error de lectura
        printk("Error: failed reading button state");
        return;

    } else if (pin_state == 0) {  // Botón soltado
        k_timer_stop(&long_pulse_timer);

        if (!state->long_press_detected) {
            uint16_t mbut = SHORT_PRESS;
            k_msgq_put(&but_q, &mbut, K_NO_WAIT);
        }

        state->long_press_detected = false;

    } else { // Boton presionado
        state->long_press_detected = false;
        k_timer_start(&long_pulse_timer, K_SECONDS(1), K_NO_WAIT);
    }
}

static void long_pulse_handler(struct k_timer *timer)
{
    ARG_UNUSED(timer);
    int pin_state = gpio_pin_get_dt(&button); // recogemos si está activo o no
    struct button_state *state = k_timer_user_data_get(timer);

    if (pin_state == 1){  // Boton sigue pulsado
        k_timer_stop(&debounce_timer);
        uint16_t mbut = LONG_PRESS;
        k_msgq_put(&but_q, &mbut, K_NO_WAIT);
        state->long_press_detected = true;

    } else { // Error de lectura
        printk("Error: failed reading button state");
        return;
    }

}


static void button_isr(const struct device *port,
                       struct gpio_callback *cb,
                       gpio_port_pins_t pins)
{   
    ARG_UNUSED(port);
    ARG_UNUSED(cb);
    ARG_UNUSED(pins);

    // al tratarse de una interrupcion la comprobacion del gpio y del boton debe hacerse fuera 
    // no hay que hacerla directamente en la funcion como si haciamos en los threads  

    k_timer_start(&debounce_timer, K_MSEC(25), K_NO_WAIT);

    
}

int init_button(void)
{
    k_timer_user_data_set(&debounce_timer, &btn_state);
    k_timer_user_data_set(&long_pulse_timer, &btn_state);

    if (!gpio_is_ready_dt(&button)) {
        return -ENODEV;
    }

    // Configurar como entrada
    int ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
    if (ret != 0) return ret;

    // Interrupción en ambos flancos para capturar pulsado y liberado
    ret = gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_BOTH);  //en vez de edge to activate both, asi devuelve o 0 o 1
    if (ret != 0) return ret;

    // Asignar button_isr al callback
    gpio_init_callback(&button_cb_data, button_isr, BIT(button.pin));
    gpio_add_callback(button.port, &button_cb_data);

    return 0;
}