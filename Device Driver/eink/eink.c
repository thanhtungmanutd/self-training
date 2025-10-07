#include "eink.h"

static void wait_until_idle(eink_t *ctx) {
    u8 state;
    do {
        state = gpiod_get_value(ctx->busy);
    } while (state == 1);
}

static void frame_px(eink_t *ctx, u16 x, u16 y, u8 font_col) {
    u16 off;
    u16 pos;

    pos = (y * (EINK_DISPLAY_WIDTH / 8 )) + (x / 4);
    off = (3 - (x % 4)) * 2;
    ctx->p_frame[pos] &= ~(0x03 << off);
    ctx->p_frame[pos] |= ((font_col & 0x03) << off);
}

static void char_wr(eink_t *ctx, u16 ch_idx) {
    u8   ch_width = 0;
    u8   x_cnt;
    u8   y_cnt;
    u16  x = 0;
    u16  y = 0;
    u16  tmp;
    u8   temp = 0;
    u8   mask = 0;
    u32  offset;
    const u8 *ch_table;
    const u8 *ch_bitmap;

    if (ch_idx < ctx->dev_font.first_char) {
        return;
    }
    if (ch_idx > ctx->dev_font.last_char) {
        return;
    }

    offset = 0;
    tmp = (ch_idx - ctx->dev_font.first_char) << 2;
    ch_table = (const u8*) (ctx->dev_font.p_font + (8 + tmp ));
    ch_width = *ch_table;

    offset = (u32)ch_table[1] + ((u32)ch_table[2] << 8) + ((u32)ch_table[3] << 16);

    ch_bitmap = ctx->dev_font.p_font + offset;

    if ((ctx->dev_font.orientation == EINK_FO_HORIZONTAL) ||
        (ctx->dev_font.orientation == EINK_FO_VERTICAL_COLUMN)) {
        y = ctx->dev_cord.y;
        for(y_cnt = 0; y_cnt < ctx->dev_font.height; y_cnt++) {
            x = ctx->dev_cord.x;
            mask = 0;
            for (x_cnt = 0; x_cnt < ch_width; x_cnt++) {
                if (!mask) {
                    temp = *ch_bitmap++;
                    mask = 0x01;
                }

                if (temp & mask) {
                    frame_px(ctx, x, y, ctx->dev_font.color);
                    udelay(80);
                }

                x++;
                mask <<= 1;
            }
            y++;
        }

        if (ctx->dev_font.orientation == EINK_FO_HORIZONTAL) {
            ctx->dev_cord.x = x + 1;
        } else {
            ctx->dev_cord.y = y;
        }
    } else {
        y = ctx->dev_cord.x;

        for (y_cnt = 0; y_cnt < ctx->dev_font.height; y_cnt++) {
            x = ctx->dev_cord.y;
            mask = 0;

            for (x_cnt = 0; x_cnt < ch_width; x_cnt++) {
                if (mask == 0) {
                    temp = *ch_bitmap++;
                    mask = 0x01;
                }

                if (temp & mask) {
                    frame_px(ctx, x, y, ctx->dev_font.color);
                    udelay(80);
                }
                x--;
                mask <<= 1;
            }
            y++;
        }
        ctx->dev_cord.y = x - 1;
    }
}

static void display_delay(void) {
    mdelay(1);
    mdelay(1);
}

int eink_init(eink_t *ctx) {
    ctx->spidev->mode = SPI_MODE_0;
	int ret = spi_setup(ctx->spidev);
	if (ret < 0) {
		dev_err(&ctx->spidev->dev, "spi_setup failed!\n");
		return ret;
	}

    gpiod_direction_output(ctx->cs, 1);
	gpiod_direction_output(ctx->rst, 1);
    gpiod_direction_output(ctx->dc, 1);
    gpiod_direction_input(ctx->busy);

    return 0;
}

void eink_send_cmd(eink_t *ctx, u8 command) {
    gpiod_set_value(ctx->dc, 0);
    gpiod_set_value(ctx->cs, 0);
    spi_write(ctx->spidev, &command, 1 );
    gpiod_set_value(ctx->cs, 1);  
}

void eink_send_data(eink_t *ctx, u8 c_data) {
    gpiod_set_value(ctx->dc, 1);
    gpiod_set_value(ctx->cs, 0);
    spi_write(ctx->spidev, &c_data, 1 );
    gpiod_set_value(ctx->cs, 1);  
}

void eink_reset(eink_t *ctx) {
    gpiod_set_value(ctx->rst, 0);
    mdelay(100);
    mdelay(100);
    gpiod_set_value(ctx->rst, 1);
    mdelay(100);
    mdelay(100);
}

void eink_sleep_mode(eink_t *ctx) {
    eink_send_cmd(ctx, EINK_CMD_DEEP_SLEEP_MODE);
    wait_until_idle(ctx) ;
}

void eink_set_lut(eink_t *ctx, const u8 *lut, u8 n_bytes) {
    u8 cnt;
    
    eink_send_cmd(ctx, EINK_CMD_WRITE_LUT_REGISTER);
    
    for (cnt = 0; cnt < n_bytes; cnt++ ) {
        eink_send_data( ctx, lut[ cnt ]);
    }
}

void eink_start_config(eink_t *ctx) {
    eink_reset(ctx);
    eink_send_cmd(ctx, EINK_CMD_DRIVER_OUTPUT_CONTROL);
    eink_send_data(ctx, ((EINK_DISPLAY_HEIGHT - 1) & 0xFF));
    eink_send_data(ctx, (((EINK_DISPLAY_HEIGHT - 1) >> 8) & 0xFF));
    eink_send_data(ctx, 0x00);                                                  
    eink_send_cmd(ctx, EINK_CMD_BOOSTER_SOFT_START_CONTROL );
    eink_send_data(ctx, 0xD7);
    eink_send_data(ctx, 0xD6);
    eink_send_data(ctx, 0x9D);
    eink_send_cmd(ctx, EINK_CMD_WRITE_VCOM_REGISTER);
    eink_send_data(ctx, 0xA8);                                                  
    eink_send_cmd(ctx, EINK_CMD_SET_DUMMY_LINE_PERIOD);
    eink_send_data(ctx, 0x1A);                                                  
    eink_send_cmd(ctx, EINK_CMD_SET_GATE_TIME);
    eink_send_data(ctx, 0x08);                                                  
    eink_send_cmd(ctx, EINK_CMD_DATA_ENTRY_MODE_SETTING);
    eink_send_data(ctx, 0x03);                                                  

    display_delay();
}

void eink_set_mem_pointer(eink_t *ctx, u8 x, u8 y) {
    eink_send_cmd(ctx, EINK_CMD_SET_RAM_X_ADDRESS_COUNTER);
    eink_send_data(ctx, ( x >> 3 ) & 0xFF);
    eink_send_cmd(ctx, EINK_CMD_SET_RAM_Y_ADDRESS_COUNTER);
    eink_send_data(ctx, y & 0xFF);
    eink_send_data(ctx, (y >> 8) & 0xFF);
    wait_until_idle(ctx);
}

void eink_set_mem_area(eink_t *ctx, eink_xy_t *xy) {
    eink_send_cmd(ctx, EINK_CMD_SET_RAM_X_ADDRESS_START_END_POSITION);
    eink_send_data(ctx, (xy->x_start >> 3) & 0xFF);
    eink_send_data(ctx, (xy->x_end >> 3) & 0xFF);
    eink_send_cmd(ctx, EINK_CMD_SET_RAM_Y_ADDRESS_START_END_POSITION);
    eink_send_data(ctx, xy->y_start & 0xFF);
    eink_send_data(ctx, (xy->y_start >> 8) & 0xFF);
    eink_send_data(ctx, xy->y_end & 0xFF);
    eink_send_data(ctx, (xy->y_end >> 8) & 0xFF);
}

void eink_update_display(eink_t *ctx) {
    // Silicon Labs fixed: 23/1/2025
    // Delay_100ms( );
    eink_send_cmd(ctx, EINK_CMD_DISPLAY_UPDATE_CONTROL_2);
    eink_send_data(ctx, 0xC4);
    eink_send_cmd(ctx, EINK_CMD_MASTER_ACTIVATION);
    eink_send_cmd(ctx, EINK_CMD_TERMINATE_FRAME_READ_WRITE);
    wait_until_idle(ctx);
}

void eink_fill_screen(eink_t *ctx, u8 color) {
    u16 cnt;
    eink_xy_t xy;
    
    xy.x_start = 0;
    xy.y_start = 0; 
    xy.x_end = EINK_DISPLAY_WIDTH - 1;
    xy.y_end = EINK_DISPLAY_HEIGHT - 1;
     
    eink_set_mem_area(ctx, &xy);
    eink_set_mem_pointer(ctx, 0, 0);

    eink_send_cmd(ctx, EINK_CMD_WRITE_RAM);
    for (cnt = 0; cnt < EINK_DISPLAY_RESOLUTIONS; cnt++) {
       eink_send_data( ctx, color );
    }

    display_delay();
    eink_update_display(ctx);
}

void eink_image(eink_t *ctx, const u8* image_buffer) {
    u16 cnt;

    eink_xy_t xy;
    
    xy.x_start = 0;
    xy.y_start = 0; 
    xy.x_end = EINK_DISPLAY_WIDTH - 1;
    xy.y_end = EINK_DISPLAY_HEIGHT - 1;
    
    eink_set_mem_area( ctx, &xy );
    eink_set_mem_pointer( ctx, 0, 0 );

    eink_send_cmd(ctx, EINK_CMD_WRITE_RAM);

    for (cnt = 0; cnt < EINK_DISPLAY_RESOLUTIONS; cnt++) {
        eink_send_data(ctx, image_buffer[cnt]);
    }

    display_delay();
    eink_update_display(ctx);
}

void eink_text(eink_t *ctx, char *text, eink_text_set_t *text_set) {
    u16 cnt;
    eink_xy_t xy;

    if((text_set->text_x >= EINK_DISPLAY_WIDTH) || (text_set->text_y >= EINK_DISPLAY_HEIGHT)) {
        return;
    }

    ctx->dev_cord.x = text_set->text_x;
    ctx->dev_cord.y = text_set->text_y;

    for(cnt = 0; cnt < text_set->n_char; cnt++) {
        char_wr( ctx, text[ cnt ] );
    }
    
    xy.x_start = 0;
    xy.y_start = 0; 
    xy.x_end = EINK_DISPLAY_WIDTH - 1;
    xy.y_end = EINK_DISPLAY_HEIGHT - 1;
    
    eink_set_mem_area(ctx, &xy);
    eink_set_mem_pointer(ctx, 0, 0);

    eink_send_cmd(ctx, EINK_CMD_WRITE_RAM);

    for(cnt = 0; cnt < EINK_DISPLAY_RESOLUTIONS; cnt++) {
        eink_send_data(ctx, ctx->p_frame[cnt]);
    }

    display_delay();
    eink_update_display(ctx);
}

void eink_set_font(eink_t *ctx, eink_font_t *cfg_font)
{
    ctx->dev_font.p_font        = cfg_font->p_font;
    ctx->dev_font.first_char    = cfg_font->p_font[ 2 ] + ( cfg_font->p_font[ 3 ] << 8 );
    ctx->dev_font.last_char     = cfg_font->p_font[ 4 ] + ( cfg_font->p_font[ 5 ] << 8 );
    ctx->dev_font.height        = cfg_font->p_font[ 6 ];
    ctx->dev_font.color         = cfg_font->color;
    ctx->dev_font.orientation   = cfg_font->orientation;
}