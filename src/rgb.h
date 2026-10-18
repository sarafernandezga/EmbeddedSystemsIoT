#ifndef RGB_H
#define RGB_H

#include <zephyr/kernel.h>
#include <stdint.h>

/* Leds Modes */
enum rgb_modes {
    OFF_LED,
    BLUE_LED,
    RED_LED,
    GREEN_LED,
    YELLOW_LED,
    PURPLE_LED,
    CIAN_LED,
    WHITE_LED
};

/* Message Queue ID */
extern struct k_msgq rgb_q;

#endif