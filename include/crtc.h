#ifndef CRTC_H
#define CRTC_H

#include "types.h"

typedef struct {
  u8 h_total;
  u8 c_per_row;
  u8 h_sync_pos;
  u8 sync_width;
  u8 v_total;
  f32 v_adjust;
  u8 n_rows;
  u8 v_sync_pos;
  u8 idr;
  u8 scan_lines_per_c;
  u8 cursor_start;
  u8 cursor_end;
  u16 upper_left_character_mem_loc;
  u16 curr_cursor;
  u16 l_pen_pos;

  u8 targeted_reg;
} CRTC;

extern CRTC crtc;

u8 crtc_read(u16 addr);
void crtc_write(u16 addr, u8 value);
void dump_crtc();

#endif // !CRTC_H
