#include <zephyr.h>
#include <devicetree.h>
#include <drivers/gpio.h>
#include <lcd.hpp>
#include <stopwatchperipherals.hpp>

StopWatchPeripherals peripherals;

/* Message queue for the sw0 pressed state for pressed button
*  true     = button is pressed down
*  false    = button is released
*/
K_MSGQ_DEFINE(sw0_pressed_bool_lcd, sizeof(bool), 2, 0);
K_MSGQ_DEFINE(sw0_pressed_bool_led, sizeof(bool), 2, 0);

/*Defines for initializing threads*/
K_THREAD_STACK_DEFINE(t0_stack_area, 2048);
K_THREAD_STACK_DEFINE(t1_stack_area, 2048);

struct k_thread t0_data;
struct k_thread t1_data;


/*Helper variables for the callback*/
bool pressed = false;
uint32_t pressed_time;


/**
 * @brief callback for the buttonpress
 * 
 * When pressed down: Get uptime and put it in the two queues. 
 * Also put new state to the pressed_bool queues
 * 
 * @param port  part of the callback function syntax
 * @param cb    part of the callback function syntax    
 * @param pin   part of the callback function syntax
 */
void handle_button_pressed_down(const struct device* port, struct gpio_callback* cb, gpio_port_pins_t pin){
    
    if(pressed){
        pressed = false;
        k_msgq_put(&sw0_pressed_bool_lcd, &pressed, K_NO_WAIT);
        k_msgq_put(&sw0_pressed_bool_led, &pressed, K_NO_WAIT);
    }
    else{
        
        pressed = true;
        k_msgq_put(&sw0_pressed_bool_lcd, &pressed, K_NO_WAIT);
        k_msgq_put(&sw0_pressed_bool_led, &pressed, K_NO_WAIT);

    }

    

}


void main(void)
{

    
    peripherals.init(handle_button_pressed_down); //Init peripherals by passing the callback function
    
    
    
    k_tid_t t0_tid = k_thread_create(   &t0_data, t0_stack_area,
                                        K_THREAD_STACK_SIZEOF(t0_stack_area),
                                        lcd_run,
                                        (void*)&sw0_pressed_bool_lcd, NULL, NULL,   
                                        2, 0, K_MSEC(1000));
    
   k_tid_t t1_tid = k_thread_create(   &t1_data, t1_stack_area,
                                        K_THREAD_STACK_SIZEOF(t1_stack_area),
                                        run_leds,
                                        (void*)&peripherals, (void*)&sw0_pressed_bool_led, NULL,
                                        2, 0, K_MSEC(1000));
                       


    while(true){
        k_msleep(1000);
    }

}
