#include "adc.h"

#include <stdbool.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/sys/printk.h>
#include <errno.h>

/* ADC Variables */
#define ADC_NODE DT_PATH(zephyr_user)

static const struct adc_dt_spec adc_ch =
    ADC_DT_SPEC_GET_BY_IDX(ADC_NODE, 0);

static int16_t sample;
static int percentage;

K_SEM_DEFINE(sem_adc_gate, 0, 1);

/* Message Queue */
K_MSGQ_DEFINE_TYPE(adc_q, struct adc_m , 1);

/* ADC timer callback */
static void adc_timer_handler(struct k_timer *timer)
{
    ARG_UNUSED(timer);

    /* Wakes up ADC thread to take a measurement */
    k_sem_give(&sem_adc_gate);
}

K_TIMER_DEFINE(adc_timer, adc_timer_handler, NULL);

/* ADC Initialization */
static int initialize_adc(struct adc_sequence *sequence)
{
    int ret;

    /* Check if the ADC device is ready */
    if (!adc_is_ready_dt(&adc_ch)) {
        printk("ADC: device not prepared\n");
        return -ENODEV;
    }

    /* Configure the ADC channel */
    ret = adc_channel_setup_dt(&adc_ch);
    if (ret < 0) {
        printk("ADC: configuration error: %d\n", ret);
        return ret;
    }

    /* Initialize the ADC sequence */
    ret = adc_sequence_init_dt(&adc_ch, sequence);
    if (ret < 0) {
        printk("ADC: error initializing sequence: %d\n", ret);
        return ret;
    }

    return ret;
}

/* ADC Measurement */
/* Reads the ADC value and sends the result to the message queue */
static int take_measurement(struct adc_sequence *sequence){
    /* Take raw measurement from ADC */
    int ret = adc_read_dt(&adc_ch, sequence);

    if (ret != 0) {
        printk("ADC: Error reading: %d\n", ret);
        return ret;
    }

    /* Convert the ADC value to a percentage */
    percentage = 100 - ((int)sample * 100 / 4095);

    /* Update ADC message */
    struct adc_m madc = {
        .raw = (int)sample,
        .percentage = percentage
    };
    
    /* Send the measurement to the message queue */
    k_msgq_put(&adc_q, &madc, K_NO_WAIT);

    return ret;
}

/* ADC Pause Control */
void set_state_adc(bool pause)
{

    if (pause) {                 // Activate pause mode
        /* Stop measurement timer */
        k_timer_stop(&adc_timer);

    } else {                    // Resume measurementes  
        /* Take a measure and restart timer */
        k_sem_give(&sem_adc_gate);
        k_timer_start(&adc_timer, K_SECONDS(2), K_SECONDS(2));

    }
}

/* Thread ADC */
void my_thread_adc(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    int ret;

     /* Variables */
    struct adc_sequence sequence = {
        .buffer = &sample,
        .buffer_size = sizeof(sample),
    };

    /* Initialize ADC */
    ret = initialize_adc(&sequence);

    if (ret < 0) {
        printk("ADC: error de inicializacion: %d\n", ret);
        return;
    }

    /* Initialize measures */
    set_state_adc(false);

    while (1) {
        /* Wait for a measurement request */
        k_sem_take(&sem_adc_gate, K_FOREVER);
        take_measurement(&sequence);

    }
}

/* Start ADC thread */
K_THREAD_DEFINE(
    my_thread_adc_id,  /* Thread identifier */
    1024,              /* Stack size */
    my_thread_adc,     /* Entry function */
    NULL, NULL, NULL,  /* Parameters */
    4,                 /* Priority */
    0,                 /* Options */
    0                  /* Start delay */
);