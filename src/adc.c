#include "adc.h"

#include <stdbool.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/sys/printk.h>
#include <errno.h>

/* ADC variables */
#define ADC_NODE DT_PATH(zephyr_user)

static const struct adc_dt_spec adc_ch =
    ADC_DT_SPEC_GET_BY_IDX(ADC_NODE, 0);

/* Data variables */
static int16_t sample;
static int percentage;

/* Semaphore */
K_SEM_DEFINE(sem_adc_gate, 0, 1);

/* Message Queue */
K_MSGQ_DEFINE_TYPE(adc_q, struct adc_m , 1);

/* Initialize ADC */
static int initialize_adc(struct adc_sequence *sequence)
{
    int ret;

    if (!adc_is_ready_dt(&adc_ch)) {
        printk("ADC: device not prepared\n");
        return -ENODEV;
    }

    ret = adc_channel_setup_dt(&adc_ch);
    if (ret < 0) {
        printk("ADC: configuratio error: %d\n", ret);
        return ret;
    }

    ret = adc_sequence_init_dt(&adc_ch, sequence);
    if (ret < 0) {
        printk("ADC: error initializing secuence: %d\n", ret);
        return ret;
    }

    return ret;
}

/* Obtiene una medida, calcula el porcentaje y lo manda por cola de mensajes */
static int take_measurement(struct adc_sequence *sequence){
    /* Toma una medida del ADC */
    int ret = adc_read_dt(&adc_ch, sequence);

    if (ret == 0) {   /* Si la medida es correcta */

        /* Calcula el porcentaje de brillo */
        percentage = 100 - ((int)sample * 100 / 4095);

        /* Actualiza la estructura */
        struct adc_m madc = {
            .raw = (int)sample,
            .percentage = percentage
        };

        /* La manda por la cola de mensajes */
        k_msgq_put(&adc_q, &madc, K_NO_WAIT);
        
    } else {   /* Sino hay un error de lectura */
        printk("ADC: error de lectura: %d\n", ret);

    }

    return ret;
}

/* Thread ADC */
void my_thread_adc(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    int ret;

    /* Se crea la estructura de secuencia */
    struct adc_sequence sequence = {
        .buffer = &sample,
        .buffer_size = sizeof(sample),
    };

    /* Inicializa el ADC */
    ret = initialize_adc(&sequence);

    while (1) {
        /* Espera indefinidamente hasta que el manager le de permiso */
        k_sem_take(&sem_adc_gate, K_FOREVER); 

        /* Toma una medida y la manda por la cola de mensajes */
        ret = take_measurement(&sequence);

    }
}


/* Start ADC thread */
K_THREAD_DEFINE(
    my_thread_adc_id,  /* Identificador */
    1024,              /* Stack size */
    my_thread_adc,  /* Funcion de entrada */
    NULL, NULL, NULL,  /* Parametros */
    4,                 /* Prioridad */
    0,                 /* Opciones */
    0                  /* Retardo de inicio */
);

