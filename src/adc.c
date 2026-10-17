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

/* Message Queue */
K_MSGQ_DEFINE_TYPE(adc_q, struct adc_m , 1);

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

static int take_measurement(struct adc_sequence *sequence){
    int ret = adc_read_dt(&adc_ch, sequence);

    if (ret == 0) {

        int percentage = 100 - ((int)sample * 100 / 4095);

        // calculamos todo en madc (despues del percentage para que no de error de compilacion)
        struct adc_m madc = {
            .raw = (int)sample,
            .percentage = percentage
        };

        k_msgq_put(&adc_q, &madc, K_NO_WAIT);
        
    } else {
        printk("ADC: error de lectura: %d\n", ret);

    }

    return ret;
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

        ret = take_measurement(&sequence);

        k_sleep(K_SECONDS(2));
    }
}


/* Start adc thread */
K_THREAD_DEFINE(
    my_thread_adc_id,  /* Identificador */
    1024,              /* Stack size */
    my_thread_adc,  /* Funcion de entrada */
    NULL, NULL, NULL,  /* Parametros */
    4,                 /* Prioridad */
    0,                 /* Opciones */
    0                  /* Retardo de inicio */
);

