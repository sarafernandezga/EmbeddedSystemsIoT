# Hands On Lab — Ambient Light Monitoring System

## 1. Project Overview

This project implements an ambient light monitoring system using **Zephyr RTOS**. The system reads light intensity through an ADC and controls an RGB LED according to the measured brightness.

The application uses multiple threads and synchronization mechanisms to manage the sensor, LED, and user input. The operating mode can be changed using button B2.

## 2. State Machine Diagrams

The state machine contains three states: NORMAL, BLUE and OFF.

<img width="3676" height="2088" alt="state_machine_embedded drawio(1)" src="https://github.com/user-attachments/assets/c08a847f-7bec-4e98-8d6c-804b4cf41cf3" />

<br>
<br>
<br>
<br>

**State behavior:**

- **NORMAL:** 
  - Measure ambient light intensity using the ADC every 2 seconds.
  - Calculate brightness as a percentage of the ADC full-scale range.
  - Set the RGB LED color according to the measured brightness:
    - RED: brightness below 33%.
    - YELLOW: brightness between 33% and 66%, inclusive.
    - GREEN: brightness above 66%.
  - Print the current mode and ADC readings to the serial terminal.
  - **Transitions:** Short press switches to **BLUE**; Long press switches to **OFF**.

- **BLUE:** 
  - Keep the RGB LED blue.
  - The serial terminal prints `System BLUE`.
  - Suspend ADC measurements until NORMAL mode is restored.
  - **Transitions:** Short press returns to **NORMAL**; Long press switches to **OFF**.

- **OFF:**
  - In OFF mode, the RGB LED is turned off and ADC measurements are suspended.
  - The serial terminal prints `System OFF`.
  - **Transitions:** A short press restores **NORMAL** mode. A long press keeps the system in **OFF** mode.
  - *Note:* The long-press event is detected as soon as the 1-second threshold is reached, without waiting for button release, and releasing the button after a long press does not trigger an additional short-press event.

## 4. Software Architecture

The application is organized into separate source files to improve modularity and maintainability.

The proposed responsibilities are:

- **`main.c`** — Application initialization and startup.
- **`button.c`** — Button B2 handling, including short-press and long-press detection.
- **`adc.c`** — ADC initialization, periodic sampling, and brightness calculation.
- **`rgb.c`** — RGB LED initialization and color control.
- **`mode_manager.c`** — Operating-mode management and state transitions.

The final thread organization and communication mechanisms will be documented according to the implemented design.

## 5. Hardware Requirements

The project is implemented on an STM32WL Nucleo-64 board running Zephyr RTOS.

### Pin Connections

The following table describes the hardware connections used by the application.

| Component | Signal | Board Pin | Description |
|---|---|---|---|
| RGB LED | Red | PB5 | Controls red channel |
| RGB LED | Green | PC1 | Controls green channel |
| RGB LED | Blue | PC2 | Controls blue channel |
| KY-018 LDR  | ADC | PB1 | Analog input / Measures light intensity |

## 5. Code Description

This section describes the implemented software modules, explains how they work, and tracks the development progress of the application.

### 5.1. RGB LED Control Module (`rgb.c` / `rgb.h`)

**Status: Implemented.**

The RGB LED module controls the red, green, and blue channels through GPIO pins using a dedicated Zephyr thread.

**Devicetree Overlay Configuration**

The overlay defines the three LED channels under the `rgb_leds` node using the `gpio-leds` compatible.

The following aliases provide access to the GPIO specifications from `rgb.c`:

| Alias | GPIO pin | Description |
|---|---|---|
| `rgbred` | PB5 | Red channel |
| `rgbgreen` | PC1 | Green channel |
| `rgbblue` | PC2 | Blue channel |

All three channels use `GPIO_ACTIVE_LOW`. Zephyr's `gpio_pin_set_dt()` API handles this polarity automatically, allowing the application to use logical values of `1` to turn a channel on and `0` to turn it off.

**Thread Architecture**

The `my_thread_rgb()` thread is created using `K_THREAD_DEFINE()`. During initialization, it configures the three GPIO pins as outputs, initially inactive.

The thread then waits for color commands from the `rgb_q` message queue and calls `set_color()` whenever a new command arrives.

**Message Queue**

The `rgb_q` queue is defined in `rgb.c` using `K_MSGQ_DEFINE()`. It stores one pending message of type `enum rgb_modes`, representing the requested LED color.

Other modules, such as the mode manager, can request a color change by sending a color constant to the queue using `k_msgq_put()`.

**Supported Colors**

| Constant | LED output |
|---|---|
| `OFF_LED` | All channels off |
| `RED_LED` | Red |
| `GREEN_LED` | Green |
| `BLUE_LED` | Blue |
| `YELLOW_LED` | Red + Green |
| `PURPLE_LED` | Red + Blue |
| `CIAN_LED` | Green + Blue |
| `WHITE_LED` | Red + Green + Blue |

**Color Control**

The `set_color()` function first turns off all three channels and then activates the GPIO outputs required for the selected color. If an invalid color mode is received, the function prints an error message.

**Thread Synchronization**

The RGB thread calls `k_msgq_get()` with `K_FOREVER`, blocking until a color command arrives. This avoids continuously polling the queue and reduces unnecessary CPU usage.

### 5.2. Light Control Module (`adc.c` / `adc.h`)

**Status: Implemented.**

The ADC module reads the analog signal generated by the Light Dependent Resistor (LDR), converts the raw measurement into a light percentage, and sends the results to other application threads using a message queue.

**Devicetree Overlay Configuration**

The ADC channel is configured in `.overlay` under the `&adc1` node. Channel 5 is configured with a 12-bit resolution, unity gain, internal reference voltage and default acquisition time.

The `zephyr,user` node exposes the ADC channel through the `io-channels` property:

```dts
/ {
    zephyr,user {
        io-channels = <&adc1 5>;
    };
};
```

In `adc.c`, `DT_PATH(zephyr_user)` identifies the `zephyr,user` node, while `ADC_DT_SPEC_GET_BY_IDX()` retrieves the first ADC channel specified by its `io-channels` property. This allows the module to access the ADC configuration without hardcoding the channel settings in the C code.

**ADC Initialization**

The `initialize_adc()` function prepares the ADC for measurements. It checks whether the ADC device is ready using `adc_is_ready_dt()`, configures the selected channel with `adc_channel_setup_dt()`, and initializes the measurement sequence using `adc_sequence_init_dt()`.

If any initialization step fails, the function prints an error message and returns the corresponding error code.

**Thread Architecture**

The module defines the `my_thread_adc()` thread using `K_THREAD_DEFINE()`. The thread initializes the ADC sequence and waits for measurement requests using `k_sem_take()` on `sem_adc_gate`.

A Zephyr timer, defined with `K_TIMER_DEFINE()`, triggers measurements every two seconds. Its callback releases the semaphore, allowing the ADC thread to perform the next measurement. This avoids continuous polling while keeping the measurement interval controlled by the timer.

**Message Queue**

The `adc_q` queue is defined in `adc.c` using `K_MSGQ_DEFINE_TYPE()`. It stores one pending message containing an `adc_m` structure, which is defined in `adc.h`.

The structure contains two fields:

- `raw`: the raw ADC reading.
- `percentage`: the calculated light percentage.

The measurement is sent to the queue using `k_msgq_put()` with `K_NO_WAIT`, allowing the ADC thread to continue without waiting if the queue is full. Other threads can retrieve the measurement from `adc_q` for further processing.

**Taking Measurements**

The `take_measurement()` function reads the ADC using `adc_read_dt()`, storing the result in the `sample` variable.

If the conversion succeeds and the ADC is not paused, the function calculates the light percentage using the following formula:

```c
percentage = 100 - ((int)sample * 100 / 4095);
```

The value 4095 represents the maximum reading for a 12-bit ADC. The formula converts the raw reading into a percentage and inverts the result, so higher ADC readings correspond to lower percentages.

The calculated percentage represents the ambient light level: a higher percentage indicates greater ambient light intensity. As the light intensity increases, the LDR resistance decreases, causing the voltage at the ADC input to decrease. This relationship is reflected in the inverted percentage calculation.

The raw value and calculated percentage are stored in an `adc_m` structure and sent through the message queue. If the ADC conversion fails, an error message is printed.

**Pause and Resume Control**

The `set_state_adc()` function controls whether measurements are paused. When pause mode is activated, the timer is stopped and measurements are not sent to the queue. When pause mode is disabled, the function releases the semaphore to request an immediate measurement and restarts the timer with a two-second interval.

### 5.3. Button Input Module (`button.c` / `button.h`)

**Status: Implemented.**

The button module detects button presses and distinguishes between short and long presses. It uses GPIO interrupts and a Zephyr timer to detect long presses and sends button events to other application threads through a message queue.

**Devicetree Overlay Configuration**

The button is accessed through the `userbt` alias, which must be defined in the Devicetree configuration and point to the corresponding button node.

In `button.c`, the following definition retrieves the button's GPIO configuration:

```c
static const struct gpio_dt_spec button =
    GPIO_DT_SPEC_GET(DT_ALIAS(userbt), gpios);
```

This allows the module to access the button's GPIO port and pin through the Devicetree configuration, without hardcoding the hardware details in the C code.

**Button Initialization**

The `init_button()` function initializes the button and its interrupt handling. It checks whether the GPIO device is ready using `gpio_is_ready_dt()`, configures the pin as an input with `gpio_pin_configure_dt()`, and registers `button_isr()` as the GPIO callback.

Finally, it enables interrupts on both edges using `GPIO_INT_EDGE_BOTH`, allowing the application to detect both button presses and releases. The function returns `0` if initialization succeeds or an error code if a step fails.

**Interrupt Handling**

The `button_isr()` function is called when a GPIO interrupt occurs. It sends a `BUTTON_EVENT` notification to `button_evt_q` using `k_msgq_put()` with `K_NO_WAIT`.

The callback does not perform the complete button-state processing. Instead, the button thread processes the event and reads the current GPIO state using `gpio_pin_get_dt()`.

**Long-Press Detection**

The `long_timer` is configured as a one-shot timer with a timeout of one second. When the button is pressed, the thread starts the timer using `k_timer_start()`.

If the timer expires, `long_pulse_handler()` sends a `LONG_PRESS_EXPIRED` event to `button_evt_q`. The button thread then registers a `LONG_PRESS` event in `but_q` and sets the `long_set` flag to prevent the subsequent release from generating a `SHORT_PRESS` event.

When the button is released, the thread stops the long-press timer. If no long press has been detected, it sends a `SHORT_PRESS` event to `but_q`. The `long_set` flag is then reset for the next button interaction.

**Message Queues**

The module defines two message queues:

- `button_evt_q`: receives internal button events from the GPIO interrupt callback and the long-press timer callback. It stores one pending event of type `enum button_event`.
- `but_q`: transfers the detected button mode to other application threads. It stores one pending event of type `enum button_mode`.

The event types are defined as follows:

| Constant | Description |
|---|---|
| `BUTTON_EVENT` | Indicates a button state change |
| `LONG_PRESS_EXPIRED` | Indicates that the long-press timer has expired |
| `SHORT_PRESS` | Button pressed and released before the long-press threshold |
| `LONG_PRESS` | Button held until the one-second timer expires |

Events are sent using `k_msgq_put()` with `K_NO_WAIT`, allowing the callbacks and thread to continue without waiting if a queue is full.

**Thread Architecture**

The module defines `my_thread_but()` using `K_THREAD_DEFINE()`. The thread initializes the button and then waits for internal events using `k_msgq_get()` with `K_FOREVER`.

When an event arrives, the thread reads the GPIO state or processes the long-press timeout. This event-driven design avoids continuously polling the button and allows the application to distinguish between short and long presses.


### 5.4. Operating Mode Manager (`mode_manager.c` / `mode_manager.h`)

**Status: Implemented.**

The mode manager coordinates the application's behavior by processing button events, managing the system operating mode, and controlling the RGB LED output according to ambient light measurements received from the ADC module.

**Operating Modes**

The system defines three operating modes in `mode_manager.h`:

| Mode | Description |
|---|---|
| `NORMAL` | Processes ambient light measurements and updates the LED color according to the measured light level. |
| `BLUE` | Sets the RGB LED to blue and pauses ADC measurements. |
| `OFF` | Turns off the RGB LED and pauses ADC measurements until the system is reactivated. |

The current mode is stored in the `event` variable inside `my_thread_manager()`.

**Thread Architecture**

The `my_thread_manager()` thread is created using `K_THREAD_DEFINE()`. It acts as the central coordinator of the application, processing button events and ADC measurements in a continuous loop.

The thread checks the button message queue using `k_msgq_get()` with a timeout of 50 milliseconds. If a button event is received, the manager processes it according to the current operating mode.

**Button Event Processing**

The mode manager retrieves button events from `but_q`, which is defined in the button module. The events are represented by `SHORT_PRESS` and `LONG_PRESS`.

The system transitions between modes according to the following rules:

| Current mode | Button event | New mode | Action |
|---|---|---|---|
| `NORMAL` | `SHORT_PRESS` | `BLUE` | Sets the LED to blue and pauses ADC measurements. |
| `NORMAL` | `LONG_PRESS` | `OFF` | Turns off the LED and pauses ADC measurements. |
| `BLUE` | `SHORT_PRESS` | `NORMAL` | Resumes ADC measurements and returns to automatic light monitoring. |
| `BLUE` | `LONG_PRESS` | `OFF` | Turns off the LED and pauses ADC measurements. |
| `OFF` | `SHORT_PRESS` | `NORMAL` | Resumes ADC measurements and returns to automatic light monitoring. |
| `OFF` | `LONG_PRESS` | `OFF` | Remains in the OFF mode. |

When a mode transition requires an LED update, the manager sends the corresponding color command to `rgb_q`. It also prints a message to the serial console to indicate the active operating mode.

**ADC Measurement Processing**

The manager processes ADC measurements only when the current mode is `NORMAL`. It retrieves available measurements from `adc_q` using `k_msgq_get()` with `K_NO_WAIT`, so it does not block while waiting for new ADC data.

When a measurement is available, the manager selects the appropriate LED color according to the following thresholds:

| Ambient light percentage | LED color |
|---|---|
| Below 33% | Red |
| From 33% to 66%, inclusive | Yellow |
| Above 66% | Green |

The selected color is sent to `rgb_q`, where the RGB module applies the corresponding GPIO outputs.

**Message Queue Communication**

The mode manager communicates with the button, ADC, and RGB modules through their message queues:

- `but_q`: receives short-press and long-press events from the button module.
- `adc_q`: receives raw ADC readings and calculated ambient light percentages.
- `rgb_q`: sends color commands to the RGB LED module.

This message-based architecture allows the modules to operate independently while the manager coordinates their behavior.

When the system transitions back to `NORMAL`, the manager calls `k_msgq_purge(&adc_q)` to remove pending ADC measurements. This helps prevent older readings from being processed immediately after the mode changes.