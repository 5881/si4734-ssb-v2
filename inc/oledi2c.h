/*
 * Библиотека работы с i2c oled дисплеем на SSD1306 
 */
#ifndef SSD1306_H
#define SSD1306_H

#define OLEDADDR 0x3C
#define OLEDI2C I2C1
#define CHAR_ON_SCREEN 168
#define F0_FAMILY 1


void oled_init(void);
void oled_send_cmd(uint8_t cmd);
void oled_send_cmd2(uint8_t *cmd, uint16_t n);
void oled_send_data(uint8_t data);
void oled_send_data2(uint8_t *data, uint16_t n);
void oled_send_n_bytes_data(uint8_t byte, uint8_t n);

void oled_set_col_block(uint8_t start_col,uint8_t end_col, uint8_t start_row, uint8_t end_row);
void oled_draw_char_at(uint8_t col, uint8_t row, uint8_t ch, uint8_t inv);
void oled_draw_char_x2_at(uint8_t col, uint8_t row, uint8_t ch,	uint8_t inv);
void oled_draw_char_x3_at(uint8_t col, uint8_t row, uint8_t ch,	uint8_t inv);
void oled_ascii_tst(void);
void oled_string_at(unsigned char col,unsigned char row, unsigned char *chr,	uint8_t inv);
void oled_string_x2_at(unsigned char col,unsigned char row,	unsigned char *chr,	uint8_t inv);
void oled_string_x3_at(unsigned char col,unsigned char row,	unsigned char *chr,	uint8_t inv);
void oled_send_char(char ch);
#endif
