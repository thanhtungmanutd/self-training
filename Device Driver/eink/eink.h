#ifndef _EINK_H_
#define _EINK_H_

#include <linux/types.h>
#include <linux/spi/spi.h>
#include <linux/gpio/consumer.h>
#include <linux/delay.h> 

#define EINK_DISPLAY_WIDTH                                   200
#define EINK_DISPLAY_HEIGHT                                  200
#define EINK_DISPLAY_RESOLUTIONS                             5000

#define EINK_SCREEN_COLOR_WHITE                              0xFF
#define EINK_SCREEN_COLOR_BLACK                              0x00
#define EINK_SCREEN_COLOR_LIGHT_GREY                         0xAA
#define EINK_SCREEN_COLOR_DARK_GREY                          0x55

#define EINK_FO_HORIZONTAL                                   0x00
#define EINK_FO_VERTICAL                                     0x01
#define EINK_FO_VERTICAL_COLUMN                              0x02

#define EINK_CMD_DRIVER_OUTPUT_CONTROL                       0x01
#define EINK_CMD_BOOSTER_SOFT_START_CONTROL                  0x0C
#define EINK_CMD_GATE_SCAN_START_POSITION                    0x0F
#define EINK_CMD_DEEP_SLEEP_MODE                             0x10
#define EINK_CMD_DATA_ENTRY_MODE_SETTING                     0x11
#define EINK_CMD_SW_RESET                                    0x12
#define EINK_CMD_TEMPERATURE_SENSOR_CONTROL                  0x1A
#define EINK_CMD_MASTER_ACTIVATION                           0x20
#define EINK_CMD_DISPLAY_UPDATE_CONTROL_1                    0x21
#define EINK_CMD_DISPLAY_UPDATE_CONTROL_2                    0x22
#define EINK_CMD_WRITE_RAM                                   0x24
#define EINK_CMD_WRITE_VCOM_REGISTER                         0x2C
#define EINK_CMD_WRITE_LUT_REGISTER                          0x32
#define EINK_CMD_SET_DUMMY_LINE_PERIOD                       0x3A
#define EINK_CMD_SET_GATE_TIME                               0x3B
#define EINK_CMD_BORDER_WAVEFORM_CONTROL                     0x3C
#define EINK_CMD_SET_RAM_X_ADDRESS_START_END_POSITION        0x44
#define EINK_CMD_SET_RAM_Y_ADDRESS_START_END_POSITION        0x45
#define EINK_CMD_SET_RAM_X_ADDRESS_COUNTER                   0x4E
#define EINK_CMD_SET_RAM_Y_ADDRESS_COUNTER                   0x4F
#define EINK_CMD_TERMINATE_FRAME_READ_WRITE                  0xFF

typedef struct {
    const u8 *p_font;
    u16      color;
    u8       orientation;
    u16      first_char;
    u16      last_char;
    u16      height;
} eink_font_t;

typedef struct {
    u16 x;
    u16 y;
} eink_cordinate_t;

typedef struct {
    u8 x_start;
    u8 y_start; 
    u8 x_end;
    u8 y_end;
} eink_xy_t;

typedef struct {
    u8 text_x;
    u8 text_y;
    u8 n_char;
} eink_text_set_t;

typedef struct {
    // Output pins 
    struct gpio_desc *cs;
    struct gpio_desc *rst;
    struct gpio_desc *dc;

    // Input pins 
    struct gpio_desc *busy;
    
    // Modules 
    struct spi_device *spidev;
    
    eink_font_t dev_font;
    eink_cordinate_t dev_cord;
    u8 p_frame[5000];
} eink_t;

int eink_init(eink_t *ctx);
void eink_send_cmd(eink_t *ctx, u8 command);
void eink_send_data(eink_t *ctx, u8 c_data);
void eink_reset(eink_t *ctx);
void eink_sleep_mode(eink_t *ctx);
void eink_set_lut(eink_t *ctx, const u8 *lut, u8 n_bytes);
void eink_start_config(eink_t *ctx);
void eink_set_mem_pointer(eink_t *ctx, u8 x, u8 y);
void eink_set_mem_area(eink_t *ctx, eink_xy_t *xy);
void eink_update_display(eink_t *ctx);
void eink_fill_screen(eink_t *ctx, u8 color);
void eink_image(eink_t *ctx, const u8* image_buffer);
void eink_text(eink_t *ctx, char *text, eink_text_set_t *text_set);
void eink_set_font(eink_t *ctx, eink_font_t *cfg_font);

#endif /* _EINK_H_ */