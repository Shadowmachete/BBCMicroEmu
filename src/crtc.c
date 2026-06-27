/*
 *  CRTC Module
 *
 *  R0: h_total, total number of displayed + non-displayed character time-units
 *      per row
 *  R1: c_per_row, number of displayed characters per row
 *  R2: h_sync_pos, number of character widths offset from LHS of screen
 *      (increase -> shift left, decrease -> shift right)
 *  R3: sync_width
 *    bits 0-3: h sync width (typically not changed)
 *    bits 4-7: v sync width (set to 2 in all modes)
 *  R4: v_total, integer number of character lines
 *  R5: v_adjust, fractional part to be used with R4
 *  R6: n_rows
 *  R7: v_sync_pos, sync position with respect to the reference (same as
 *      h_sync_pos?)
 *  R8: interlace and delay register, raster scan mode and cursor / display
 *      delay
 *    bits 0-1: interlace modes (strange diagram
 *         https://archive.org/details/bbc-micro-advanced-user-guide/page/364/mode/2up)
 *      00 && 10: normal
 *      01: interlace sync
 *      11: interlace sync and video
 *    bits 4-5: display blanking delay
 *    bits 6-7: cursor blanking delay
 *      00: no delay
 *      01: 1 character delay
 *      10: 2 character delay
 *      11: disable video / cursor output
 *  R9: scan_lines_per_c, value is 1 less than total number of output scan lines
 *  R10: cursor_start
 *    bit 7: unused
 *    bit 6: enable / disable blink
 *    bit 5: blink timing
 *      0: 16 times field rate
 *      1: 32 times field rate
 *    bit 0-4: cursor start line
 *  R11: cursor_end, cursor end line
 *  R12-R13: upper_left_character_mem_loc, 8 low order (R13) and 6 high order
 *           (R12) bits
 *  R14-R15: curr_cursor, 8 low order (R15) and 6 high order (R14)
 *           bits
 *  R16-R17: l_pen_pos, unused by us
 */

#include "crtc.h"
#include <stdio.h>

CRTC crtc;

u8 crtc_read(u16 addr) {
  if (addr == 0xFE00)
    return 0x00;

  switch (crtc.targeted_reg) {
  case 14: {
    return crtc.curr_cursor >> 8;
  };
  case 15: {
    return (u8)crtc.curr_cursor;
  };
  }

  return 0x00;
}

void crtc_write(u16 addr, u8 value) {
  if (addr == 0xFE00) {
    crtc.targeted_reg = value;
    return;
  }

  switch (crtc.targeted_reg) {
  case 0: {
    crtc.h_total = value;
  } break;
  case 1: {
    crtc.c_per_row = value;
  } break;
  case 2: {
    crtc.h_sync_pos = value;
  } break;
  case 3: {
    crtc.sync_width = value;
  } break;
  case 4: {
    crtc.v_total = value;
  } break;
  case 5: {
    crtc.v_adjust = value;
  } break;
  case 6: {
    crtc.n_rows = value;
  } break;
  case 7: {
    crtc.v_sync_pos = value;
  } break;
  case 8: {
    crtc.idr = value;
  } break;
  case 9: {
    crtc.scan_lines_per_c = value;
  } break;
  case 10: {
    crtc.cursor_start = value;
  } break;
  case 11: {
    crtc.cursor_end = value;
  } break;
  case 12: {
    crtc.upper_left_character_mem_loc =
        (crtc.upper_left_character_mem_loc & 0x00FF) | ((value & 0x3F) << 8);
  } break;
  case 13: {
    crtc.upper_left_character_mem_loc =
        (crtc.upper_left_character_mem_loc & 0xFF00) | value;
  } break;
  case 14: {
    crtc.curr_cursor = (crtc.curr_cursor & 0x00FF) | ((value & 0x3F) << 8);
  } break;
  case 15: {
    crtc.curr_cursor = (crtc.curr_cursor & 0xFF00) | value;
  } break;
  case 16: {
    crtc.l_pen_pos = (crtc.l_pen_pos & 0x00FF) | ((value & 0x3F) << 8);
  } break;
  case 17: {
    crtc.l_pen_pos = (crtc.l_pen_pos & 0xFF00) | value;
  } break;
  default: {
    printf("Invalid crtc register %d to change", crtc.targeted_reg);
  }
  }
}

void dump_crtc() {
  printf("h_total: %d\n", crtc.h_total);
  printf("c_per_row: %d\n", crtc.c_per_row);
  printf("h_sync_pos: %d\n", crtc.h_sync_pos);
  printf("sync_width: %d\n", crtc.sync_width);
  printf("v_total: %d\n", crtc.v_total);
  printf("v_adjust: %f\n", crtc.v_adjust);
  printf("n_rows: %d\n", crtc.n_rows);
  printf("v_sync_pos: %d\n", crtc.v_sync_pos);
  printf("idr: %d\n", crtc.idr);
  printf("scan_lines_per_c: %d\n", crtc.scan_lines_per_c);
  printf("cursor_start: %d\n", crtc.cursor_start);
  printf("cursor_end: %d\n", crtc.cursor_end);
  printf("upper_left_character_mem_loc: %d\n",
         crtc.upper_left_character_mem_loc);
  printf("curr_cursor: %d\n", crtc.curr_cursor);
  printf("l_pen_pos: %d\n", crtc.l_pen_pos);
}
