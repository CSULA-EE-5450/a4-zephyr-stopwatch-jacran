#ifndef HD44780_H
#define HD44780_H

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include <stdint.h>
#include <drivers/gpio.h>

/* LCD shield pinout uses DT specs because GPIO ports differ by pin
 *  Ard Usage
 *  A0  Buttons
 *  D4  DB4
 *  D5  DB5
 *  D6  DB6
 *  D7  DB7
 *  D8  RS
 *  D9  EN
 */

#define HD44780_PIN_D4 GPIO_DT_SPEC_GET(DT_NODELABEL(hd44780_pin_d4), gpios)
#define HD44780_PIN_D5 GPIO_DT_SPEC_GET(DT_ALIAS(dfr0009d5), gpios)
#define HD44780_PIN_D6 GPIO_DT_SPEC_GET(DT_ALIAS(dfr0009d6), gpios)
#define HD44780_PIN_D7 GPIO_DT_SPEC_GET(DT_ALIAS(dfr0009d7), gpios)
#define HD44780_PIN_RS GPIO_DT_SPEC_GET(DT_ALIAS(dfr0009rs), gpios)
#define HD44780_PIN_EN GPIO_DT_SPEC_GET(DT_ALIAS(dfr0009en), gpios)

enum hd44780_pins
{
    D4,
    D5,
    D6,
    D7,
    RS,
    EN,
    PINS_MAX
};

enum hd44780_cmds
{
    HD44780_CMD_CLEAR = 1,
    HD44780_CMD_HOME = 2,
    HD44780_CMD_MODE = 4,
        HD44780_MODE_INC = 2,
        HD44780_MODE_DEC = 0,
        HD44780_MODE_SHIFT = 1,
    HD44780_CMD_ONOFF = 8,
        HD44780_ONOFF_DISP_ON = 4,
        HD44780_ONOFF_DISP_OFF = 0,
        HD44780_ONOFF_CURS_ON = 2,
        HD44780_ONOFF_CURS_OFF = 0,
        HD44780_ONOFF_BLINK_ON = 1,
        HD44780_ONOFF_BLINK_OFF = 0,
    HD44780_CMD_SHIFT = 16,
        HD44780_SHIFT_DISP = 8,
        HD44780_SHIFT_CURS = 0,
        HD44780_SHIFT_RIGHT = 4,
        HD44780_SHIFT_LEFT = 0,
    HD44780_CMD_CONFIG = 32,
        HD44780_CONFIG_DATA8 = 16,
        HD44780_CONFIG_DATA4 = 0,
        HD44780_CONFIG_2LINES = 8,
        HD44780_CONFIG_1LINE0 = 0,
        HD44780_CONFIG_5X11 = 4,
        HD44780_CONFIG_5X8 = 0,
    HD44780_CMD_CGRAM = 64,
    HD44780_CMD_DDRAM = 128,
};

struct hd44780_display
{
    struct gpio_dt_spec pin_dt[PINS_MAX];
};


void hd44780_init();
void hd44780_cmd(uint8_t cmd, uint8_t flags);
void hd44780_data(char val);
void hd44780_pos(uint8_t row, uint8_t col);


#ifdef __cplusplus
}
#endif

#endif // HD44780_H
