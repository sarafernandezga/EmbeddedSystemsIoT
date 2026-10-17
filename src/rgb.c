#include "rgb.h"

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>

/* Message Queue */
K_MSGQ_DEFINE_TYPE(rgb_q, sizeof(uint32_t), 1);

/* Leds Variables */
static const struct gpio_dt_spec rgb_blue = GPIO_DT_SPEC_GET(DT_ALIAS(rgbblue), gpios);
static const struct gpio_dt_spec rgb_red = GPIO_DT_SPEC_GET(DT_ALIAS(rgbred), gpios);
static const struct gpio_dt_spec rgb_green = GPIO_DT_SPEC_GET(DT_ALIAS(rgbgreen), gpios);

/* Functions */
static void set_color(uint32_t mode);

/* Initialize leds */
static int initialize_RGB(const struct gpio_dt_spec *pin_rgb)
{
    if (!device_is_ready(pin_rgb->port)) {
        printk("RGB: LED device not ready\n");
        return -1;
    }

    if (gpio_pin_configure_dt(pin_rgb, GPIO_OUTPUT_INACTIVE) < 0) {
        printk("RGB: error configuring LED\n");
        return -1;
    }

    return 0;
}

/* Thread leds */
void my_thread_rgb(void *p1, void *p2, void *p3){
    static uint32_t mode;

    initialize_RGB(&rgb_blue);
    initialize_RGB(&rgb_red);
    initialize_RGB(&rgb_green);

    while (1) {

        /* K_FOREVER suspends the thread indefinitely until a message arrives, saving CPU cycles. */
        /* Using K_NO_WAIT would cause a continuous loop */
        if (k_msgq_get(&rgb_q, &mode, K_FOREVER) == 0) {
                set_color(mode);

        } else{
            printk("Error: Reading message queue");
        }
    }
}

/* Set appropiate color */
static void set_color(uint32_t mode)
{
    /* Primero apagamos todo */
    gpio_pin_set_dt(&rgb_blue, 0);
    gpio_pin_set_dt(&rgb_red, 0);
    gpio_pin_set_dt(&rgb_green, 0);

    /* Después encendemos el color deseado */
    if (mode == BLUE_LED) {
        gpio_pin_set_dt(&rgb_blue, 1);
    }
    else if (mode == GREEN_LED) {
        gpio_pin_set_dt(&rgb_green, 1);
    }
    else if (mode == RED_LED) {
        gpio_pin_set_dt(&rgb_red, 1);
    }
    else if (mode == YELLOW_LED) {
        gpio_pin_set_dt(&rgb_red, 1);
        gpio_pin_set_dt(&rgb_green, 1);

    } else if (mode == PURPLE_LED) {
        gpio_pin_set_dt(&rgb_red, 1);
        gpio_pin_set_dt(&rgb_blue, 1);
    }
    else if (mode == CIAN_LED) {
        gpio_pin_set_dt(&rgb_blue, 1);
        gpio_pin_set_dt(&rgb_green, 1);
    } 
}


/* Crear el thread */
K_THREAD_DEFINE(
    rgb_led_thread_id,  /* Identificador */
    1024,               /* Stack */
    my_thread_rgb,      /* Función de entrada: nombre correcto */
    NULL, NULL, NULL,   /* Parámetros */
    5,                  /* Prioridad */
    0,                  /* Opciones */
    0                   /* Retardo de inicio */
);
