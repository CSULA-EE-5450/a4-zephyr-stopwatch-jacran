#ifndef LCD_H
#define LCD_H

#include <hd44780.h>
#include <cstdlib>
#include <cstdio>

/**
 * @brief The available states for the lcd stopwatch
 * 
 */
enum states_sw{
    SW_IDLE = 0,
    SW_RUN,
    SW_RUN_W_LAP, 
    SW_PAUSE,
    SW_RESET,
    SW_EASTEREGG
};


/**
 * @brief class for controlling the hd44780 module
 * 
 * made specific for the stopwatch module
 * 
 */
class StopWatchLCD{

    public: 
        StopWatchLCD();
        void write(char* input_str, size_t input_str_len);
        void writeln(char* input_str, size_t input_str_len, uint8_t column);
        void init(void);
        void print_running_time(void);
        void display_paused_time(void);
        void remove_lap_time(void);
        void set_lap_time(uint32_t timestamp);
        void run_state(void);
        void print_scroll(uint8_t* start_index, char* input_str, uint8_t length, uint8_t column);

        uint32_t curr_time, offset_timestamp, pause_timestamp;
        uint32_t last_lap_timestamp = 0;
        uint32_t lap_time = 0;
        uint32_t pause_interval = 0;
        uint32_t total_pause_time = 0;
        uint32_t total_pause_time_since_last_lap = 0;
        bool first_lap = true;
        bool pause_occurred = false;
        uint8_t state;
        uint8_t scroll = 0;



    private:
        char lcd_column0_str[16],lcd_column1_str[16];
        char idle_str[16] = "Stopwatch ready";
        char reset_str[9] = "00:00:00";
        char reset_instr[17] = "Press to restart";


        /**
         * @brief private function for converting a given time to minutes, seconds and ms(only two digits)
         * 
         * @param time_ms the time to be converted
         * @param data_arr an array where the converted data is passed back by reference
         * 
         * data_arr[0] = minutes (MM)
         * data_arr[1] = minutes (SS)
         * data_arr[2] = minutes (mm)
         */
        void calculate_min_sec_ms_from_ms(uint32_t time_ms, uint16_t* data_arr){

            data_arr[0] = (time_ms/(60000))%60;     //MM;
            data_arr[1] = (time_ms/1000)%60;        //SS
            data_arr[2] = (time_ms%1000)/10;        //ms (div by 10 to get 2 digits)
        }
};

/*The run function for the lcd which is used by one thread. It could not be a member of a class. */
void lcd_run(void* p_msgq_pressed_state, void* unused, void* un_used);


#endif /*LCD_H*/