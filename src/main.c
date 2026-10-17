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

    // Creamos la configuración a comprobar
    uint32_t mode = GREEN_LED;      // Color rojo

    static struct adc_m madc;

    // Inicializamos el botón
    int ret = init_button();

    if (ret != 0) {
        printk("Error al inicializar el boton: %d\n", ret);
        return ret;
    }

    while (1) {
        k_sleep(K_SECONDS(3));
        
        uint16_t mbut;
        k_msgq_get(&but_q, &mbut, K_FOREVER);

        if (mbut == SHORT_PRESS){
            printk("Pulsacion corta \n");
        } else if (mbut == LONG_PRESS){
            printk("Pulsacion larga\n");
        } else{
            printk("Error: Pulsacion");
        }

         //printk("ADC: raw = %d / percentage = %d %%\n", madc.raw, madc.percentage);
    }
}
