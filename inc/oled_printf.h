/** \file printf.c
 * Simplified printf() and sprintf() implementation - prints formatted string to
 * USART (or whereever). Most common % specifiers are supported. It costs you about
 * 3k FLASH memory - without floating point support it uses only 1k ROM!
 * \author Freddie Chopin, Martin Thomas, Marten Petschke and many others
 * \date 16.2.2012
 * reduced scanf() added by Andrzej Dworzynski on the base of reduced sscanf() written by
 * some nice gay from this post: http://www.fpgarelated.com/usenet/fpga/show/61760-1.php
 * thanks to everybody:)
 * \date 12.2.2013
 */

/*****************************************************************************
 * 5 января 2019
 * Немного подправил текст библиотеки, адоптировав под libopencm3
 * Выкинул лишние файлы, уместив всё в одном.
 * 12 мая 2019
 * перепилено для работы с tft экраном
 * Shaman
 * 27 июля 2019
 * добавлена поддержка backspase '\b' в kscanf
 * Shaman
 * 17 ноября 2019
 * добавлены функции sprintf() и st_printf_at()
 * 7 апреля 2020
 * перепилено под oled дисплей
*****************************************************************************/ 
#ifndef OLEDPRINTF_H
#define OLEDPRINTF_H
int o_printf(const char *format, ...);
int o_printf_at(uint8_t col, uint8_t row,uint8_t size,
						uint8_t inv, const char *format, ...);
int sprintf(char *buffer, const char *format, ...);
int kscanf(const char* format, ...);
void set_scanf_mode(unsigned char);
//int fast_kscanf(const char* format, ...);


#endif
