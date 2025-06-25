/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * GPIO control
 *
 * Copyright (C) 2025 Elias Calzado Carvajal
 */

#include <pigpiod_if2.h>
#include <stdio.h>
#include <stdlib.h>

#include "gpio.h"

struct gpio_pin {
    int num;
    int mode;
    int state;
};

struct gpio_ctrl {
    int pi;

    gpio_stream_cb_t stream_cb;
    void *stream_ud;

    int num_pins;
    struct gpio_pin *pins[];
};

static char *strerr(int errnum)
{
    switch (errnum) {
        case PI_BAD_GPIO: return "GPIO not 0-31";
        case PI_BAD_MODE: return "mode not 0-7";
        case PI_BAD_LEVEL: return "level not 0-1";
        case PI_NOT_PERMITTED: return "GPIO operation not permitted";
        case PI_BAD_FILTER: return "bad filter parameter";
        default: return "unknown pigpio error";
    }
}

int gpio_set_pin_state(void *g, int pin_name, int state)
{
    struct gpio_ctrl *gpio = g;
    struct gpio_pin *pin = gpio->pins[pin_name];

    int ret = gpio_write(gpio->pi, pin->num, state);

    if (ret < 0) {
        printf("Failed to write GPIO: %s (%d)\n", strerr(ret), ret);
        return 1;
    }

    pin->state = state;

    return 0;
}

void gpio_set_stream_callback(struct gpio_ctrl *gpio, gpio_stream_cb_t cb, void *ud)
{
    gpio->stream_cb = cb;
    gpio->stream_ud = ud;
}

static void gpio_sensor_callback(int pi __attribute__((unused)), uint32_t pin_num __attribute__((unused)),
                uint32_t level, uint32_t tick __attribute__((unused)), void *g)
{
    struct gpio_ctrl *gpio = g;

    gpio->pins[SENSOR]->state = level;

    printf("Read sensor pin %s -> stream %s\n",
        gpio->pins[SENSOR]->state == LOW ? "LOW" : "HIGH",
        gpio->pins[SENSOR]->state == LOW ? "on" : "off");

    gpio->stream_cb(gpio->stream_ud, gpio->pins[SENSOR]->state);
}

int gpio_init(struct gpio_ctrl *gpio)
{
    /* Disable pigpio's internal signal handler. */
	//int cfg = gpioCfgGetInternals();
	//cfg |= PI_CFG_NOSIGHANDLER;
	//gpioCfgSetInternals(cfg);

	if ((gpio->pi = pigpio_start(NULL, NULL)) < 0) {
		printf("Failed to initialize GPIO\n");
		return 1;
	}

    int ret = 0;

    for (int i = 0; i < gpio->num_pins; i++) {
        if ((ret = set_mode(gpio->pi, gpio->pins[i]->num, gpio->pins[i]->mode)) < 0) {
            printf("Failed to set GPIO mode: %s (%d)\n", strerr(ret), ret);
            return 1;
        }
    }

    if (gpio_set_pin_state(gpio, RUNNING, HIGH))
        return 1;

    if ((ret = set_glitch_filter(gpio->pi, gpio->pins[SENSOR]->num, 100000)) < 0) {
        printf("Failed to set GPIO glitch filter: %s (%d)\n", strerr(ret), ret);
        return 1;
    }

    if ((ret = callback_ex(gpio->pi, gpio->pins[SENSOR]->num, EITHER_EDGE, gpio_sensor_callback, gpio)) < 0) {
        printf("Failed to set GPIO alert function: %s (%d)\n", strerr(ret), ret);
        return 1;
    }

    if ((ret = gpio_read(gpio->pi, gpio->pins[SENSOR]->num)) < 0) {
        printf("Failed to read GPIO: %s (%d)\n", strerr(ret), ret);
        return 1;
    }

    gpio->pins[SENSOR]->state = ret;
    gpio_sensor_callback(gpio->pi, gpio->pins[SENSOR]->num, gpio->pins[SENSOR]->state, 0, gpio);

    return 0;
}

struct gpio_ctrl *gpio_create(char *pinlist) {
    int num_pins = 3;
    int v[num_pins];

    if (sscanf(pinlist, "%d %d %d", &v[0], &v[1], &v[2]) != num_pins)
        return NULL;

    struct gpio_ctrl *gpio = malloc(sizeof *gpio + num_pins * sizeof *gpio->pins);
    gpio->num_pins = num_pins;

    struct gpio_pin *running = malloc(sizeof *running);
    running->num = v[0];
    running->mode = PI_OUTPUT;
    gpio->pins[0] = running;
    
    struct gpio_pin *streaming = malloc(sizeof *streaming);
    streaming->num = v[1];
    streaming->mode = PI_OUTPUT;
    gpio->pins[1] = streaming;

    struct gpio_pin *sensor = malloc(sizeof *sensor);
    sensor->num = v[2];
    sensor->mode = PI_INPUT;
    gpio->pins[2] = sensor;

    return gpio;
}

void gpio_cleanup(struct gpio_ctrl *gpio)
{
    if (gpio == NULL) 
        return;
    
    struct gpio_pin *pin;
    for (int i = 0; i < gpio->num_pins; i++) {
        pin = gpio->pins[i];
        if (pin->mode == PI_OUTPUT) {
            gpio_write(gpio->pi, pin->num, LOW);
            set_mode(gpio->pi, pin->num, PI_INPUT);
        }
        free(pin);
    }

    pigpio_stop(gpio->pi);

    free(gpio);
}