#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <stdbool.h>
#include <stdint.h>
#include "rgb.h"
#include "button.h"

/* Variables */
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(DT_ALIAS(userbt1), gpios);

/* Button Callback */
static struct gpio_callback button_cb_data;

/* Timers */
struct button_state {
    bool long_press_detected;
};

static struct button_state btn_state = {
    .long_press_detected = false
};

static void long_pulse_handler(struct k_timer *timer);


K_TIMER_DEFINE(long_pulse_timer, long_pulse_handler, NULL);

/* Message */
K_MSGQ_DEFINE_TYPE(but_q, sizeof(uint16_t), 1);


static void long_pulse_handler(struct k_timer *timer)
{
    ARG_UNUSED(timer);
    int pin_state = gpio_pin_get_dt(&button); // recogemos si está activo o no
    struct button_state *state = k_timer_user_data_get(timer);

    if (pin_state == 1){  // Boton sigue pulsado
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

    struct button_state *state = &btn_state;

    // al tratarse de una interrupcion la comprobacion del gpio y del boton debe hacerse fuera 
    // no hay que hacerla directamente en la funcion como si haciamos en los threads  

    int pin_state = gpio_pin_get_dt(&button);

    if (pin_state < 0){
        printk("Error: failed reading button state");
        return;
    } else if (pin_state == 1){
        k_timer_start(&long_pulse_timer, K_SECONDS(1), K_NO_WAIT);
    } else {
        if (!state->long_press_detected) {
            k_timer_stop(&long_pulse_timer);
            uint16_t mbut = SHORT_PRESS;
            k_msgq_put(&but_q, &mbut, K_NO_WAIT);
        } else {
            state->long_press_detected = false;
        }
    }

}

int init_button(void)
{
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