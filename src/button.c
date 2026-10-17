#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include "rgb.h"
#include "button.h"

static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(DT_ALIAS(userbt1), gpios);
static struct gpio_callback button_cb_data;

static void button_isr(const struct device *port,
                       struct gpio_callback *cb,
                       gpio_port_pins_t pins)
{   
    ARG_UNUSED(port);
    ARG_UNUSED(cb);
    ARG_UNUSED(pins);

    // al tratarse de una interrupcion la comprobacion del gpio y del boton debe hacerse fuera 
    // no hay que hacerla directamente en la funcion como si haciamos en los threads  

    static int64_t press_start_time = 0; // donde guardaremos el momento de la pulsación

    static int64_t last_time = 0; // para lidiar con los rebotes nos quedamos con la ultima vez para restar
                                  // de esta forma sabemos la distancia temporal entre la ult vez que pulsamos y la 
                                  // pulsación actual 
                                  // No muere al terminar la ISR y se queda durante todo el ciclo de vida del programa
    
    int64_t now = k_uptime_get();

    if (now - last_time < 50){
        return; // ignoramos el debounce 
    }

    last_time = now; //actualizamos la ultima pulsacion

    int pin_state = gpio_pin_get_dt(&button); // recogemos si está activo o no

    if (pin_state > 0) {
        // Se acaba de presionar: guardamos marca de tiempo inicial
        press_start_time = now;
    } 

    else if (pin_state == 0 && press_start_time > 0) {
        // Se acaba de soltar: calculamos duración
        int64_t duration = now - press_start_time;
        uint32_t color;

        if (duration >= 1000) {
            color = RED_LED;   // Pulsación larga: >= 1 segundo
        } else {
            color = BLUE_LED;  // Pulsación corta: < 1 segundo
        }

        // k_msgq_put necesita un puntero a la variable (&color)
        k_msgq_put(&rgb_q, &color, K_NO_WAIT); // Enviamos a la cola del RGB (Usamos K_NO_WAIT porque estamos en una ISR)

        press_start_time = 0; // Reiniciar estado
    }
    
}

int init_button(void)
{
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