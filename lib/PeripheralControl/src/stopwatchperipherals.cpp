/**
 * @file peripheralcontrol.cpp
 * @author Jacob A. Rangnes  (jrangne@calstatela.edu)
 * @brief Control of LED0, LED1 and SW0 (USER) on the ST Disco L475 IOT01 (B-L475E-IOT01A). 
 * @version 0.1
 * @date 2022-04-30
 * 
 * 
 */

#include "stopwatchperipherals.hpp"


/**
 * @brief init function for the peripherals
 * 
 * Sets sw0 to input
 * Sets led0, led1 to output
 * 
 * configures callback to sw0
 * 
 * @param callback callback function to be added to sw0
 */
void StopWatchPeripherals::init(gpio_callback_handler_t callback){
    gpio_pin_configure_dt(&spec_pin_led0, GPIO_OUTPUT);
    gpio_pin_configure_dt(&spec_pin_led1, GPIO_OUTPUT);
    gpio_pin_configure_dt(&spec_pin_sw0, GPIO_INPUT);

    /*Add callback to SW0 */
    gpio_pin_interrupt_configure_dt(&spec_pin_sw0, GPIO_INT_EDGE_BOTH); 
    gpio_init_callback(&sw0_callback, callback, BIT(spec_pin_sw0.pin));
    gpio_add_callback(spec_pin_sw0.port, &sw0_callback);

    this->state = P_IDLE;

}

/**
 * @brief run the current state of the object
 * 
 */
void StopWatchPeripherals::run_state(void){
    switch (this->state)
    {
    case P_IDLE:
        this->turn_on_led0();
        this->turn_on_led1();
    case P_RUN:
        this->turn_off_led0();
        this->turn_off_led1();
    case P_PAUSE:
        this->turn_off_led0();
        this->turn_off_led1();
        break;
    case P_RESET:
        this->turn_on_led0();
        this->turn_on_led1();
        break;
    default:
        this->state = P_RESET;
        break;
    }
}


/**
 * @brief function for controlling led0 and led1 according to the instructions below
 * 
 * @param p_peripherals The peripheral object 
 * @param p_msgq_pressed_state msgq with the button state (pressed or not pressed)
 * @param unused unused
 * 
 * On initialization (bootup), both of the LEDs should remain lit.
 * When the button is pressed and released within 2 seconds, turn off both LEDs
 * 
 * While the stopwatch is active, if the button is pressed and held down for at least 2 seconds, 
 * activate one of the LEDs to signal to the user that we have held for at least 2 seconds, 
 * and enter the "paused" stopwatch mode. 
 * 
 * While the stopwatch is inactive(paused), if the button is pressed and held down for at least 2 seconds, 
 * activate one of the LEDs to signal to the user that we have held for at least 2 seconds, 
 * and resume the stopwatch back to the "running" mode.
 * 
 * While the stopwatch is active, if the button is pressed and held down for at least 4 seconds, 
 * activate both of the LEDs to signal to the user that we have held for at least 4 seconds, 
 * and enter the "reset" stopwatch mode.
 */
void run_leds(void* p_peripherals, void* p_msgq_pressed_state, void* unused){

    StopWatchPeripherals* peripherals = (StopWatchPeripherals*)p_peripherals;
    k_msgq* pressed_state_msgq = (k_msgq*)p_msgq_pressed_state;

    bool pressed_state = false;
    const uint16_t BUTTON_PRESSED_INTERVAL = 2000; //ms (2 sec)
    uint8_t num_times_expired = 0;

    uint32_t timer_status;

    struct k_timer button_pressed_p_timer;
    struct k_timer button_check_timer;

    k_timer_init(&button_pressed_p_timer,NULL, NULL);
    k_timer_init(&button_check_timer,NULL, NULL);

    k_timer_start(&button_check_timer, K_MSEC(1), K_FOREVER);

    
    while(true){
        
        if(k_msgq_get(pressed_state_msgq, &pressed_state, K_NO_WAIT) == 0){ //Successful read- start button-timer
            if(pressed_state){
                k_timer_start(&button_pressed_p_timer, K_MSEC(BUTTON_PRESSED_INTERVAL), K_MSEC(BUTTON_PRESSED_INTERVAL));
                num_times_expired = 0;
            }
            //Meaning button was just released - check how many times button_timer has exceeded its time 
            //      0:  less than 2 sec
            //      1:  Button pushed between 2 and 4 seconds
            //      >=2: Button pushed for at least 4 seconds
            else{
                timer_status = k_timer_status_get(&button_pressed_p_timer);
                peripherals->turn_off_led0();
                peripherals->turn_off_led1();
                if(timer_status == 0){
                    if(peripherals->state == P_RESET){
                        peripherals->state = P_RUN;
                    }
                }
                if(timer_status >= 2){
                    if(peripherals->state == P_IDLE){
                        peripherals->state = P_RESET;
                        peripherals->run_state();
                    }
                }
                k_timer_stop(&button_pressed_p_timer);
            }
        }

        if(k_timer_status_get(&button_check_timer) > 0){
            if(pressed_state){ //button is still pushed down
                timer_status = k_timer_status_get(&button_pressed_p_timer);
                if(timer_status > 0){
                    num_times_expired += 1;
                }
                if(num_times_expired == 1){
                    peripherals->turn_on_led0();
                }else if (num_times_expired == 2){
                    peripherals->turn_on_led1();
                }
            }
            k_timer_start(&button_check_timer, K_MSEC(1), K_FOREVER);
        }
        k_msleep(1); //had to add some sleep ?
    }
}