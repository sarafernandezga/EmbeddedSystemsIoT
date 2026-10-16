#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include "rgb.h"
#include "adc.h"

int main(void)
{
    printk("Embedded Platforms and Communications for IoT\n");
    printk("        ETSIST - UPM - MUIoT 2026-2027       \n\n");
	
    printk("    Board LED toggle (single thread: main)   \n");

    // Creamos la configuración a comprobar
    uint32_t mode = RED_LED;      // Color rojo

    static struct adc_m madc;

    // Enviamos el mensaje a la cola 'rgb_q' definida en rgb.c
    // Esto despertará al hilo my_thread_rgb
    //k_msgq_put(&rgb_q, &mode, K_NO_WAIT);

    while (1) {
        k_sleep(K_SECONDS(3));
        
        mode = (mode % 6) + 1; // Rotacion entre colores
        k_msgq_put(&rgb_q, &mode, K_NO_WAIT);

        
        k_msgq_get(&adc_q, &madc, K_FOREVER);
         printk("ADC: raw = %d / percentage = %d %%\n", madc.raw, madc.percentage);
    }
}
