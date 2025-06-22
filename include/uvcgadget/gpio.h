/*
 * GPIO control
 */
#ifndef __GPIO_H__
#define __GPIO_H__

#define HIGH 1
#define LOW 0

enum gpio_pin_name { 
    RUNNING,
    STREAMING,
    SENSOR
};

struct gpio_ctrl;

typedef void (*gpio_stream_cb_t)(void *userdata, int state);

int gpio_set_pin_state(void *g, int pin_name, int state);

void gpio_set_stream_callback(struct gpio_ctrl *gpio, gpio_stream_cb_t cb, void *ud);

int gpio_init(struct gpio_ctrl *gpio);

struct gpio_ctrl *gpio_create(char *pinlist);

void gpio_cleanup(struct gpio_ctrl *gpio);

#endif /* __GPIO_H__ */