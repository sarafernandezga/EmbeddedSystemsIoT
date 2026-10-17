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
static volatile bool pause_mode = false;

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

        percentage = 100 - ((int)sample * 100 / 4095);

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

void set_state_adc(bool pause)
{
    // If the user uses this function to pause it activates the pause_mode
    // and if it uses to set it false it also activates the semaphore so it 
    // gets wake
    if (pause) {
        pause_mode = true; 
    } else {
        if (pause_mode){ // THis check is to make sure the pause_mode it was enabled
            pause_mode = false;
            k_sem_give(&sem_adc_gate); // It wakes up the thread
        }
    }
}

/* Thread ADC */
void my_thread_adc(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    int ret;

    struct adc_sequence sequence = {
        .buffer = &sample,
        .buffer_size = sizeof(sample),
    };

    ret = initialize_adc(&sequence);

    printk("ADC: Initialized");

    while (1) {

        if (pause_mode){
            printk("Pause mode");
            // Wait for the semaphore to open
            k_sem_take(&sem_adc_gate, K_FOREVER); 
        }

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

