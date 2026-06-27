#ifndef VIA_H
#define VIA_H

#include "types.h"

typedef struct {
  u8 reg_a;
  u8 reg_b;
  u8 ddrb;
  u8 ddra;
  u8 timer1_counterlow;
  u8 timer1_counterhigh;
  u8 timer1_latchlow;
  u8 timer1_latchhigh;
  u8 timer2_counterlow;
  u8 timer2_counterhigh;
  u8 shift_reg;
  u8 acr;
  u8 pcr;
  u8 ifr;
  u8 ier;
} VIA;

extern VIA system_via;
extern VIA user_via;

u8 system_via_read(u16 addr);
void system_via_write(u16 addr, u8 value);

u8 user_via_read(u16 addr);
void user_via_write(u16 addr, u8 value);

u16 get_vram_base();

void dump_system_via();

#endif // !VIA_H
