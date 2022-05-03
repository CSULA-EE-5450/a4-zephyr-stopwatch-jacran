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
 * @brief the main function which ensured the LCD displays the correct information given the buttonpresses
 * 
 * @param p_msgq_time time msgq containing timestamp for buttonpress
 * @param p_msgq_pressed_state sw0 pressed state msgq 
 * @param unused - not used
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
void lcd_run(void* p_msgq_time, void* p_msgq_pressed_state, void* unused){

    
    StopWatchLCD lcd = StopWatchLCD();

    k_msgq* pressed_time_msgq = (k_msgq*)p_msgq_time;
    k_msgq* pressed_state_msgq = (k_msgq*)p_msgq_pressed_state;

    char idle_str[] = "Stopwatch ready";
    char reset_str[] = "00:00:00";
    char reset_instr[] = "Press to restart";

    uint32_t pressed_time, uptime;
    

    uint8_t current_state = SW_IDLE;
    bool first_pause = true;
    bool two_sec = false;
    bool four_sec = false;
    bool pressed_state = false;

    while(true){
        if(k_msgq_get(pressed_state_msgq, &pressed_state, K_NO_WAIT) == 0){ //Successful read
            if(pressed_state){
                two_sec = four_sec = false; //reset
                k_msgq_get(pressed_time_msgq, &pressed_time, K_NO_WAIT); //get pressed time
            }else{ //Meaning button was just released
                if(four_sec == false && four_sec == false){ //Button was released within 2 seconds
                    if(current_state == SW_IDLE){
                        current_state = SW_RUN;
                        lcd.offset_timestamp = k_uptime_get_32();
                    }else if (current_state == SW_RUN)
                    {
                        
                        if(!two_sec){lcd.set_lap_time(k_uptime_get_32());}
                    }else if (current_state == SW_RESET)
                    {
                        current_state = SW_RUN;
                        lcd.offset_timestamp = k_uptime_get_32();
                        lcd.total_pause_time = lcd.last_lap_timestamp = lcd.total_pause_time_since_last_lap = 0;
                        lcd.first_lap = true;
                    }
                }
            }
        }

        if(pressed_state){
            uptime = k_uptime_get_32() - pressed_time;
            if(uptime/1000 >= 4){
                four_sec = true;
                if(current_state == SW_RUN || current_state == SW_PAUSE ){
                    current_state = SW_RESET;
                }
            }else if (uptime/1000 >= 2)
            {
                if(!two_sec){ //should only happen once per button press
                    two_sec = true;
                    if(current_state == SW_RUN){
                        lcd.pause_timestamp = lcd.curr_time;
                        current_state = SW_PAUSE;
                    }else if (current_state == SW_PAUSE){ //Resume stopwatch

                        lcd.pause_interval = (k_uptime_get() - lcd.pause_timestamp);
                        lcd.total_pause_time += lcd.pause_interval;
                        current_state = SW_RUN;
                        if(lcd.pause_occurred){ //If another pause has occurred without new lap time generated
                            lcd.total_pause_time_since_last_lap += lcd.pause_interval;
                        }else{
                            lcd.total_pause_time_since_last_lap = lcd.pause_interval;
                        }
                        
                        lcd.pause_occurred = true; 
                    }
                }
            }
        }

        if(current_state == SW_IDLE){
            lcd.writeln(idle_str, sizeof(idle_str)-1, 0);
            
        }else if (current_state == SW_RUN){
            lcd.curr_time = k_uptime_get_32();
            lcd.print_running_time();

            first_pause = true;
        }else if (current_state == SW_PAUSE)
        {
            
            lcd.display_paused_time();
        }else if (current_state == SW_RESET)
        {
            lcd.writeln(reset_str,sizeof(reset_str)-1, 0);
            lcd.writeln(reset_instr,sizeof(reset_instr)-1, 1); 
            lcd.remove_lap_time();
            lcd.pause_interval = 0;
        }
        
        
        k_msleep(50);
    }
}

