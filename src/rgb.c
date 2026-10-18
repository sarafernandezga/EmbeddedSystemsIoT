#include "rgb.h"

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>

/* Message Queue */
K_MSGQ_DEFINE(rgb_q, sizeof(enum rgb_modes), 1, 4);

/* Leds Variables */
static const struct gpio_dt_spec rgb_blue = GPIO_DT_SPEC_GET(DT_ALIAS(rgbblue), gpios);
static const struct gpio_dt_spec rgb_red = GPIO_DT_SPEC_GET(DT_ALIAS(rgbred), gpios);
static const struct gpio_dt_spec rgb_green = GPIO_DT_SPEC_GET(DT_ALIAS(rgbgreen), gpios);

/* Functions */
static void set_color(enum rgb_modes mode);

/* Initialize LEDs */
static int initialize_RGB(const struct gpio_dt_spec *pin_rgb)
{
    if (!device_is_ready(pin_rgb->port)) {
        printk("RGB: LED device not ready\n");
        return -1;
    }

    /* Configure the LED pin as output, initially inactive */
    if (gpio_pin_configure_dt(pin_rgb, GPIO_OUTPUT_INACTIVE) < 0) {
        printk("RGB: error configuring LED\n");
        return -1;
    }

    return 0;
}

/* Thread leds */
void my_thread_rgb(void *p1, void *p2, void *p3){
    static enum rgb_modes mode;

    /* Initialize the three RGB pins */
    if (initialize_RGB(&rgb_blue) != 0 ||
        initialize_RGB(&rgb_red) != 0 ||
        initialize_RGB(&rgb_green) != 0) 
    {
        printk("RGB: Initialization failed\n");
        return;
    }

    while (1) {

         /* Wait for a message */
        if (k_msgq_get(&rgb_q, &mode, K_FOREVER) == 0) {
            set_color(mode);

        } else{
            printk("RGB: Reading message queue");
        }

    }
}

/* Set the selected color */
static void set_color(enum rgb_modes mode)
{
    /* Turn off all LEDs before selecting the new color */
    gpio_pin_set_dt(&rgb_blue, 0);
    gpio_pin_set_dt(&rgb_red, 0);
    gpio_pin_set_dt(&rgb_green, 0);

    /* Turn on the LEDs required for the selected color */
    switch (mode) {
        case OFF_LED:
            break;

        case RED_LED:
            gpio_pin_set_dt(&rgb_red, 1);
            break;

        case GREEN_LED:
            gpio_pin_set_dt(&rgb_green, 1);
            break;

        case BLUE_LED:
            gpio_pin_set_dt(&rgb_blue, 1);
            break;

        case YELLOW_LED:
            gpio_pin_set_dt(&rgb_green, 1);
            gpio_pin_set_dt(&rgb_red, 1);
            break;

        case PURPLE_LED:
            gpio_pin_set_dt(&rgb_red, 1);
            gpio_pin_set_dt(&rgb_blue, 1);
            break;

        case CIAN_LED:
            gpio_pin_set_dt(&rgb_blue, 1);
            gpio_pin_set_dt(&rgb_green, 1);
            break;

        case WHITE_LED:
            gpio_pin_set_dt(&rgb_red, 1);
            gpio_pin_set_dt(&rgb_blue, 1);
            gpio_pin_set_dt(&rgb_green, 1);
            break;
        
        default:
            printk("RGB: Invalid color mode\n");
            break;
    }

}

/* Start RGB Thread */
K_THREAD_DEFINE(
    rgb_led_thread_id,  /* Thread identifier */
    1024,               /* Stack size */
    my_thread_rgb,      /* Entry function */
    NULL, NULL, NULL,   /* Parameters */
    5,                  /* Priority */
    0,                  /* Options */
    0                   /* Start delay */
);
