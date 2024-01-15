#ifndef INCLUDE_FRAMEBUFFER_H
#define INCLUDE_FRAMEBUFFER_H

#pragma once
#include "io.h"


#define FB_COMMAND_PORT 0x3D4
#define FB_DATA_PORT 0x3D5


#define FB_HIGH_BYTE_COMMAND 14
#define FB_LOW_BYTE_COMMAND 15


#define COLOR_BLACK 0
#define COLOR_LIGHT_GREY 7


#define FB_BASE_ADDRESS 0xB8000;


void outb(unsigned short port, unsigned char data);


void fb_move_cursor(unsigned short pos);


void fb_write_cell(unsigned int i, char c, unsigned char fg, unsigned char bg);


int fb_write(char *buf, unsigned int len);

#endif 