#ifndef GPIO_H
#define GPIO_H
/**
 * @defgroup GPIO GPIO functions
 * @brief Functions for button and led control.
 *
 */
/*@{*/
/**
 * @file gpio.h
 * @author Otso Jousimaa <otso@ojousima.net>
 * @date 2019-03-14
 * @copyright Ruuvi Innovations Ltd, license BSD-3-Clause.
 *
 */


/*OUTPUT GPIOs--------------------------------------------------------------*/
#define GPIO_OUTPUT_PINS_SEL_MASK                     1ULL<<CONFIG_LED_GPIO

/*INPUT GPIOs--------------------------------------------------------------*/
#define GPIO_WIFI_RESET_BUTTON_MASK                   (1ULL<<CONFIG_WIFI_RESET_BUTTON_GPIO)

/**
 * @brief sets Button as input and LED GPIO as output. 
 */
void gpio_init(void);

/**
 * @brief blinks led at 300 ms. interval
 */
void gpio_blink_led_task(void *pvParameter);

/**
 * @Brief resets WiFi configuration
 */
void gpio_timer_task(void *arg);

/**
 * @brief handles GPIO input from button
 */
void gpio_task(void* arg);

/*@}*/
#endif