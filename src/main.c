#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include "rgb.h"

int main(void)
{
    printk("Embedded Platforms and Communications for IoT\n");
    printk("        ETSIST - UPM - MUIoT 2026-2027       \n\n");
	
    printk("    Board LED toggle (single thread: main)   \n");

    // Creamos la configuración a comprobar
    struct rgb_mode inicial = {
        .mode = RED_LED,      // Color rojo
    
    };

    // Enviamos el mensaje a la cola 'rgb_q' definida en rgb.c
    // Esto despertará al hilo my_thread_rgb
    k_msgq_put(&rgb_q, &inicial, K_NO_WAIT);

    while (1) {
        k_sleep(K_SECONDS(3));
        
        inicial.mode = (inicial.mode % 6) + 1; // Rotacion entre colores
        k_msgq_put(&rgb_q, &inicial, K_NO_WAIT);
    }
}
