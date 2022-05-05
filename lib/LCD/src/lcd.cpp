/**
 * @file lcd.cpp
 * @author Jacob A. Rangnes (jrangne@calstatela.edu)
 * @brief cpp file for the StopWatch LCD. 
 * @version 0.1
 * @date 2022-05-02
 * 
 * 
 */

#include "lcd.hpp"


/**
 * @brief Construct a new StopWatchLCD::StopWatchLCD object by initing the LCD.
 * 
 */
StopWatchLCD::StopWatchLCD(){
    hd44780_init();
    hd44780_cmd(HD44780_CMD_CLEAR, 0);
    this->state = SW_IDLE;
}



/**
 * @brief write function for the hd44780
 * 
 * @param input_str the string (char arr) to be written on the lcd
 * @param input_str_len length of the string
 * 
 * this function is "borrowed" from Curtis Wang git repo:
 *      https://github.com/curtywang/zephyr-hd44780/blob/master/src/main.c
 *              
 */
void StopWatchLCD::write(char* input_str, size_t input_str_len) {
    for(size_t i = 0; i < 16; i++) {
        if (i >= input_str_len) {
            hd44780_data(' ');
        }
        else {
            hd44780_data(input_str[i]);
        }
    }
}




/**
 * @brief write a line at the specified column
 * 
 * @param input_str string to be written
 * @param input_str_len length of string to be written
 * @param column column of the lcd to write the text (must be 0 or 1). 
 */
void StopWatchLCD::writeln(char* input_str, size_t input_str_len, uint8_t column){
        hd44780_pos(column, 0);
        this->write(input_str, input_str_len);
}

/**
 * @brief helper function for displaying the stopwatch time on the lcd
 * 
 * 
 * 
 */
void StopWatchLCD::print_running_time(){
    uint16_t times[3];
    this->calculate_min_sec_ms_from_ms(curr_time - offset_timestamp - total_pause_time, times);
    sprintf(lcd_column0_str,"%02d:%02d:%02d RUNNING", times[0], times[1], times[2]);
    this->writeln(lcd_column0_str, sizeof(lcd_column0_str), 0);

}

/**
 * @brief helper function for displayinf the paused lcd info
 * 
 */
void StopWatchLCD::display_paused_time(void){
    uint16_t times[3];
    this->calculate_min_sec_ms_from_ms(curr_time - offset_timestamp - total_pause_time, times);
    sprintf(lcd_column1_str,"%02d:%02d:%02d PAUSED", times[0], times[1], times[2]);
    this->writeln(lcd_column1_str, sizeof(lcd_column1_str)-1, 0);

}

/**
 * @brief overwrite column 1 (the column for the lap time)
 * 
 * should be called when a reset of the stopwatch is needed
 * 
 */
void StopWatchLCD::remove_lap_time(void){
    this->writeln("                ", 16, 1);
}

/**
 * @brief set the new laptime and display it
 * 
 * @param timestamp timestamp sampled at the time a lap was requested
 */
void StopWatchLCD::set_lap_time(uint32_t timestamp){
    uint16_t times[3];
    uint32_t pause_offset = 0;

    if(this->pause_occurred){
        pause_offset = this->total_pause_time_since_last_lap;
        this->pause_occurred = false;
    }

    if(first_lap){
        this->lap_time = timestamp - this->offset_timestamp - pause_offset;
        first_lap = false;
    }else{
        this->lap_time = timestamp - this->last_lap_timestamp - pause_offset;
    }
    
    this->last_lap_timestamp = timestamp;

    this->calculate_min_sec_ms_from_ms(lap_time, times);
    sprintf(this->lcd_column1_str, "LAP %02d:%02d:%02d",times[0], times[1],times[2]);
    this->writeln(this->lcd_column1_str, 12, 1);
}



/**
 * @brief run the current state of the object
 * 
 */
void StopWatchLCD::run_state(void){
    switch (this->state)
    {
    case SW_IDLE:
        this->writeln(idle_str, sizeof(idle_str)-1, 0);
        break;
    case SW_RUN:
        this->curr_time = k_uptime_get_32();
        this->print_running_time();
        break;
    case SW_RUN_W_LAP:
        this->set_lap_time(this->curr_time);
        this->curr_time = k_uptime_get_32();
        this->print_running_time();
        this->state = SW_RUN;
        break;
    case SW_PAUSE:
        this->display_paused_time();
        break;
    case SW_RESET:
        this->writeln(reset_str,sizeof(reset_str)-1, 0);
        this->writeln(reset_instr,sizeof(reset_instr)-1, 1); 
        this->pause_interval = 0;
        break;
    default:
        this->state = SW_RESET;
        break;
    }
}





/**
 * @brief the main function which ensured the LCD displays the correct information given the buttonpresses
 * 
 * @param p_msgq_time time msgq containing timestamp for buttonpress
 * @param p_msgq_pressed_state sw0 pressed state msgq 
 * @param unused - not used
 * 
 * utilizes two timers. 
 *  - One for updating the lcd with a period of 50ms
 *  - One for keeping track of how long sw0 was pushed down
 * 
 * 
 * On initialization (bootup), the LCD should display "Stopwatch Ready"
 * 
 * When the button is pressed and released within 2 seconds, begin the stopwatch and display the current 
 * stopwatch time as "MM:SS:mm" (MM is minutes, SS is seconds, mm is milliseconds) on the LCD. 
 * The LCD should display "MM:SS:mm RUNNING" on the top line. Update the LCD stopwatch time roughly every 50 milliseconds.
 * 
 * While the stopwatch is active, if the button is pressed and released within 2 seconds, 
 * then you can record the last lap time. Display the last lap time on the second line. 
 * The LCD should display "LAP MM:SS:mm" on the second line.
 * 
 * While the stopwatch is active, if the button is pressed and held down for at least 2 seconds, 
 * enter the "paused" stopwatch mode. Freeze the current stopwatch time. 
 * The LCD should display "MM:SS:mm PAUSED" on the top line.
 * 
 * While the stopwatch is inactive, if the button is pressed and held down for at least 2 seconds, 
 * resume the stopwatch back to the "running" mode.
 * 
 * While the stopwatch is active, if the button is pressed and held down for at least 4 seconds, 
 * enter the "reset" stopwatch mode.
 */
void lcd_run(void* p_msgq_pressed_state, void* unused, void* un_used){

    StopWatchLCD lcd = StopWatchLCD();

    k_msgq* pressed_state_msgq = (k_msgq*)p_msgq_pressed_state;

    bool pressed_state;

    const uint8_t LCD_UPDATE_PERIOD = 50; //ms
    const uint16_t BUTTON_PRESSED_INTERVAL = 2000; //ms (2 sec)

    struct k_timer lcd_update_timer;
    struct k_timer button_pressed_timer;

    k_timer_init(&lcd_update_timer,NULL, NULL);
    k_timer_init(&button_pressed_timer,NULL, NULL);

    k_timer_start(&lcd_update_timer, K_MSEC(LCD_UPDATE_PERIOD), K_FOREVER); 


    while(true){
        if(k_msgq_get(pressed_state_msgq, &pressed_state, K_NO_WAIT) == 0){ //Successful read- start button-timer
            if(pressed_state){
                k_timer_start(&button_pressed_timer, K_MSEC(BUTTON_PRESSED_INTERVAL), K_MSEC(BUTTON_PRESSED_INTERVAL));
            }
            //Meaning button was just released - check how many times button_timer has exceeded its time 
            //      0:  less than 2 sec
            //      1:  Button pushed between 2 and 4 seconds
            //      >=2: Button pushed for at least 4 seconds
            else{
                uint32_t timer_status = k_timer_status_get(&button_pressed_timer);
                if(timer_status == 0){ //Button pressed for less than 2 sec
                    if(lcd.state == SW_IDLE){
                        lcd.state = SW_RUN;
                        lcd.offset_timestamp = k_uptime_get_32();
                    }else if (lcd.state == SW_RUN){
                        lcd.state = SW_RUN_W_LAP;
                    }else if(lcd.state == SW_RESET){
                        lcd.offset_timestamp = k_uptime_get_32();
                        lcd.total_pause_time = lcd.total_pause_time_since_last_lap = lcd.last_lap_timestamp = 0;
                        lcd.remove_lap_time();
                        lcd.first_lap = true;
                        lcd.pause_occurred = false;
                        lcd.state = SW_RUN;
                    }
                }else if (timer_status == 1){ //Button pushed between 2 and 4 seconds

                    if(lcd.state == SW_RUN){
                        lcd.pause_timestamp = lcd.curr_time;
                        lcd.state = SW_PAUSE;
                    }else if (lcd.state == SW_PAUSE){
                        lcd.pause_interval = (k_uptime_get() - lcd.pause_timestamp);
                        lcd.total_pause_time += lcd.pause_interval;

                        if(lcd.pause_occurred){ //If another pause has occurred without new lap time generated
                            lcd.total_pause_time_since_last_lap += lcd.pause_interval;
                        }else{
                            lcd.total_pause_time_since_last_lap = lcd.pause_interval;
                        }
                        
                        lcd.pause_occurred = true; 
                        lcd.state = SW_RUN;
                    }
                    
                }else if(timer_status  >= 2){ //Button pushed for at least 4 seconds
                    if(lcd.state == SW_RUN){
                        lcd.state = SW_RESET;
                    }
                }
                k_timer_stop(&button_pressed_timer);
            }
        }

        if(k_timer_status_get(&lcd_update_timer) > 0){ //Check if time to update lcd
            lcd.run_state();
            k_timer_start(&lcd_update_timer, K_MSEC(LCD_UPDATE_PERIOD), K_FOREVER);
        }
        
    }
}
