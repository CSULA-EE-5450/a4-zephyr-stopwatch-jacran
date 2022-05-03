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
    gpio_pin_configure_dt(&spec_pin_led0, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&spec_pin_led1, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&spec_pin_sw0, GPIO_INPUT);

    /*Add callback to SW0 */
    gpio_pin_interrupt_configure_dt(&spec_pin_sw0, GPIO_INT_EDGE_BOTH); 
    gpio_init_callback(&sw0_callback, callback, BIT(spec_pin_sw0.pin));
    gpio_add_callback(spec_pin_sw0.port, &sw0_callback);

}

/**
 * @brief function for controlling led0 and led1 according to the instructions below
 * 
 * @param p_peripherals The peripheral object 
 * @param p_msgq_time msgq with the timestamp for button pressed down time
 * @param p_msgq_pressed_state msgq with the button state (pressed or not pressed)
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
void run_leds(void* p_peripherals, void* p_msgq_time, void* p_msgq_pressed_state){

    StopWatchPeripherals* peripherals = (StopWatchPeripherals*)p_peripherals;
    k_msgq* pressed_time_msgq = (k_msgq*)p_msgq_time;
    k_msgq* pressed_state_msgq = (k_msgq*)p_msgq_pressed_state;

    bool two_sec = false; //To keep track if the button is pressed cont. over 2 seconds 
    bool four_sec = false; //To keep track if the button is pressed cont. over 2 seconds
    bool pressed_state = false; 
    uint32_t pressed_time, uptime;  
    uint8_t current_state = P_IDLE; //helper variable to keep track of which peripheralstate we are in
    
    while(true){
        
        /*logic for receiving and handling the messages from the queues*/
        if(k_msgq_get(pressed_state_msgq, &pressed_state, K_NO_WAIT) == 0){ //Successful read of the queue
            if(pressed_state){ //Meaning that the button was just pressed down
                two_sec = four_sec = false; //reset
                k_msgq_get(pressed_time_msgq, &pressed_time, K_NO_WAIT); //get pressed time
            }else{ //Meaning button was just released
                if(four_sec == false && four_sec == false){ //Button was released within 2 seconds
                    if(current_state == P_IDLE || current_state == P_RESET){ // P_IDLE/P_RESET --> P_RUN (sw started)
                        current_state = P_RUN;
                    }
                }
            }
        }
        
        /* logic for keeping track of the time spent pushing sw0 down*/
        if(pressed_state){
            uptime = k_uptime_get_32() - pressed_time;
            if(uptime/1000 >= 4){
                if(! four_sec){ //Should only happen once per button press
                    four_sec = true;
                    if(current_state == P_RUN || current_state == P_PAUSE ){
                        peripherals->turn_on_led1();
                        current_state = P_RESET;
                  }
                }
            }else if (uptime/1000 >= 2)
            {
                if(!two_sec){ //should only happen once per button press
                    two_sec = true;
                    if(current_state == P_RUN){
                        peripherals->turn_on_led0();
                        current_state = P_PAUSE;
                         
                        
                    }else if (current_state == P_PAUSE){ //Resume stopwatch

                        peripherals->turn_on_led0();
                        k_msleep(100);
                        current_state = P_RUN;
                        
                        
                    }
                }
            }
        }

        /*logic for turning the leds on/off correct according to the instructions*/
        if(!pressed_state){

            if(current_state == P_IDLE || current_state == P_RESET){
                peripherals->turn_on_led0();
                peripherals->turn_on_led1();
            }else if (current_state == P_RUN || current_state == P_PAUSE){
                peripherals->turn_off_led0();
                peripherals->turn_off_led1();
            }
        }
        
        k_msleep(50);
    }
}