#ifndef PERIPHERALCONTROL_H
#define PERIPHERALCONTROL_H


#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr.h>
#include <devicetree.h>
#include <drivers/gpio.h>
#include <device.h>


/**
 * @brief valid states for the peripherals
 * 
 */
enum states_p{
    P_IDLE = 0,
    P_RUN,
    P_PAUSE,
    P_RESET
};

/**
 * @brief class for controlling the peripherals needed for the stopwatch
 * 
 * - sw0
 * - led0
 * - led1
 * 
 */
class StopWatchPeripherals{
    public:
        void turn_on_led0(void) {gpio_pin_set_dt(&spec_pin_led0, 1);}
        void turn_off_led0(void) {gpio_pin_set_dt(&spec_pin_led0, 0);}
        void turn_on_led1(void) {gpio_pin_set_dt(&spec_pin_led1, 1);}
        void turn_off_led1(void) {gpio_pin_set_dt(&spec_pin_led1, 0);}
        void init(gpio_callback_handler_t callback);
        struct gpio_callback sw0_callback;
        const struct gpio_dt_spec spec_pin_led0 = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
        const struct gpio_dt_spec spec_pin_led1 = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);
        const struct gpio_dt_spec spec_pin_sw0 = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);

};

/*the function called by the thread. This could not be inside a class for some reason*/
void run_leds(void* p_peripherals, void* p_msgq_time, void* p_msgq_pressed_state);


#ifdef __cplusplus
}
#endif

#endif /* PERIPHERALCONTROL_H */