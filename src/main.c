#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include "rgb.h"
#include "adc.h"
#include "button.h"


int main(void)
{
    printk("Embedded Platforms and Communications for IoT\n");
    printk("        ETSIST - UPM - MUIoT 2026-2027       \n\n");
	
    printk("    Board LED toggle (single thread: main)   \n");

    // Initialization of the button
    int ret = init_button();

    if (ret != 0) {
        printk("Error al inicializar el boton: %d\n", ret);
        return ret;
    }

}
