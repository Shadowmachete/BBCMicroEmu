#ifndef VULA_H
#define VULA_H

#include "types.h"

typedef struct {
  u8 control_reg;
  u8 palette_reg[16];
} VULA;

extern VULA vula;

void vula_write(u16 addr, u8 value);
u8 vula_read(u16 addr);
u8 get_pixels_per_byte();
u8 get_cursor_width();
u8 is_teletext();
u8 extract_pixel(u8 byte, u8 p, u8 bpp);

void dump_vula();

#endif // !VULA_H
