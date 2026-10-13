#include "adc.h"

#include <zephyr/kernel.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/sys/printk.h>
#include <errno.h>

/* ADC Variables */

#define ADC_NODE DT_PATH(zephyr_user)

static const struct adc_dt_spec adc_ch =
    ADC_DT_SPEC_GET_BY_IDX(ADC_NODE, 0);
    

static int16_t sample;

static int initialize_adc(struct adc_sequence *sequence)
{
    int ret;

    if (!adc_is_ready_dt(&adc_ch)) {
        printk("ADC: dispositivo no preparado\n");
        return -ENODEV;
    }

    ret = adc_channel_setup_dt(&adc_ch);
    if (ret < 0) {
        printk("ADC: error de configuracion: %d\n", ret);
        return ret;
    }

    ret = adc_sequence_init_dt(&adc_ch, sequence);
    if (ret < 0) {
        printk("ADC: error inicializando secuencia: %d\n", ret);
        return ret;
    }

    return 0;
}

/* Thread ADC */
void my_thread_adc(void *p1, void *p2, void *p3)
{
    int ret;

    struct adc_sequence sequence = {
        .buffer = &sample,
        .buffer_size = sizeof(sample),
    };

    ret = initialize_adc(&sequence);

    printk("ADC: Initialized");

    while (1) {

        ret = adc_read_dt(&adc_ch, &sequence);

        if (ret == 0) {
            printk("ADC: raw = %d\n", (int)sample);
        } else {
            printk("ADC: error de lectura: %d\n", ret);
        }

        k_msleep(2000);
    }
}


/* Start leds thread */
K_THREAD_DEFINE(
    my_thread_adc_id,  /* Identificador */
    1024,              /* Stack size */
    my_thread_adc,  /* Funcion de entrada */
    NULL, NULL, NULL,  /* Parametros */
    4,                 /* Prioridad */
    0,                 /* Opciones */
    0                  /* Retardo de inicio */
);

