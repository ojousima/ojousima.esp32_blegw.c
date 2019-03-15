#include "gpio.h"
#include "driver/gpio.h"
#include "driver/timer.h"
#include "driver/periph_ctrl.h"
#include "esp_intr_alloc.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "wifi_manager.h"

#define TIMER_DIVIDER 16
#define TIMER_SCALE    (TIMER_BASE_CLK / TIMER_DIVIDER)  /*!< used to calculate counter value */
#define ESP_INTR_FLAG_DEFAULT 0

typedef struct {
    int type;  // the type of timer's event
    int timer_group;
    int timer_idx;
    uint64_t timer_counter_value;
} timer_event_t;

static xQueueHandle gpio_evt_queue = NULL;
static xQueueHandle timer_queue = NULL;

static const char TAG[] = "gpio";

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}


static void IRAM_ATTR timer_isr(void *para)
{
    uint32_t timer_flag = 1;
    xQueueSendFromISR(timer_queue, &timer_flag, NULL);
    // printf("WIFI Reset Activated......**********\n");
    TIMERG0.int_clr_timers.t0 = 1;
    // wifi_manager_disconnect_async();
}

/*
 * Initialize selected timer of the timer group 0
 *
 * timer_idx - the timer number to initialize
 * auto_reload - should the timer auto reload on alarm?
 * timer_interval_sec - the interval of alarm to set
 */
static void config_timer(void)
{
    /* Select and initialize basic parameters of the timer */
    timer_config_t config;
    config.divider = TIMER_DIVIDER;
    config.counter_dir = TIMER_COUNT_UP;
    config.counter_en = TIMER_PAUSE;
    config.alarm_en = TIMER_ALARM_EN;
    config.intr_type = TIMER_INTR_LEVEL;
    config.auto_reload = 0;


    timer_init(TIMER_GROUP_0, TIMER_0, &config);

    /* Timer's counter will initially start from value below.
       Also, if auto_reload is set, this value will be automatically reload on alarm */
    timer_set_counter_value(TIMER_GROUP_0, TIMER_0, 0x00000000ULL);

    /* Configure the alarm value and the interrupt on alarm. */
    timer_set_alarm_value(TIMER_GROUP_0, TIMER_0, 3 * TIMER_SCALE); // 3 seconds interval

    timer_enable_intr(TIMER_GROUP_0, TIMER_0);
    timer_isr_register(TIMER_GROUP_0, TIMER_0, timer_isr, (void *) TIMER_0, ESP_INTR_FLAG_IRAM, NULL);

}

void gpio_timer_task(void *arg)
{
    uint32_t timer_flag;

    for (;;)  {
        if (xQueueReceive(timer_queue, &timer_flag, portMAX_DELAY)) {

            ESP_LOGI(TAG, "WIFI Reset Activated");

            wifi_manager_disconnect_async();
        }

    }
}

void gpio_blink_led_task(void *pvParameter)
{
    for (;;)  {
        /* Blink off (output low) */
        gpio_set_level(CONFIG_LED_GPIO, 0);
        vTaskDelay(150 / portTICK_PERIOD_MS);
        /* Blink on (output high) */
        gpio_set_level(CONFIG_LED_GPIO, 1);
        vTaskDelay(150 / portTICK_PERIOD_MS);
    }
}
/**
 * Brief reacts to GPIO events
 */
void gpio_task(void* arg)
{
    uint32_t io_num;
    char timer_started = 0;

    for (;;) {
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {

            if (io_num == CONFIG_WIFI_RESET_BUTTON_GPIO)
            {
                if (timer_started == 0 && gpio_get_level(io_num) == 0)
                {
                    ESP_LOGI(TAG, "Button pressed");

                    // Start the timer

                    timer_start(TIMER_GROUP_0, TIMER_0);

                    timer_started = 1;
                }

                else
                {
                    // Stop and reinitialize the timer
                    ESP_LOGI(TAG, "Button released");

                    config_timer();

                    timer_started = 0;
                }

            }
        }
    }
}

void gpio_init(void) 
{

    gpio_pad_select_gpio(CONFIG_LED_GPIO);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(CONFIG_LED_GPIO, GPIO_MODE_OUTPUT);

    gpio_set_level(CONFIG_LED_GPIO, 0);

    /*OUTPUT GPIOs-----------------------------------------------------------------*/

    gpio_config_t io_conf;
    //disable interrupt
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set,e.g.GPIO2 for LED
    io_conf.pin_bit_mask = GPIO_OUTPUT_PINS_SEL_MASK;
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf);

    /*INPUT GPIO WIFI_RESET_BUTTON  -------------------------------------*/
    //interrupt of rising edge

    io_conf.intr_type = GPIO_PIN_INTR_ANYEDGE; //GPIO_PIN_INTR_POSEDGE; Rizwan
    //bit mask of the pins
    io_conf.pin_bit_mask = GPIO_WIFI_RESET_BUTTON_MASK;
    //set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);


    /*-------------------------------------------------------------------*/
    //create a queue to handle gpio event from isr
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    timer_queue = xQueueCreate(10, sizeof(timer_event_t));

    config_timer();

    //install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(CONFIG_WIFI_RESET_BUTTON_GPIO, gpio_isr_handler, (void*) CONFIG_WIFI_RESET_BUTTON_GPIO);
}