#ifndef RGB_H
#define RGB_H

#include <zephyr/kernel.h>
#include <stdint.h>

/* Leds Modes */
#define OFF_LED       0x00
#define BLUE_LED      0X01
#define RED_LED       0x02
#define GREEN_LED     0X03
#define YELLOW_LED    0X04
#define PURPLE_LED    0x05
#define CIAN_LED      0X06

/* Message variable */
extern struct k_msgq rgb_q;

#endif