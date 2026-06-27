/*
 *  Video ULA Module
 *
 *  0xFE20-0xFE2F
 *  20: Control Register
 *    bit 0: flash on / off
 *    bit 1: teletext / normal
 *    bit 2-3: number of characters per line
 *      00: 10 characters
 *      01: 20 characters
 *      10: 40 characters
 *      11: 80 characters
 *    bit 4: low (MODEs 4-7) / high (MODEs 0-3) clock frequency
 *    bit 5-7: cursor width
 *      000: hide cursor
 *      100: MODE 0, 3, 4, 6
 *      110: MODE 1, 5
 *      111: MODE 2
 *      010: MODE 7
 *  21: Palette Register
 *    bit 0-3: physical colour EOR 7
 *    bit 4-7: logical colour field
 */

#include <stdio.h>

#include "vula.h"

VULA vula;

void vula_write(u16 addr, u8 value) {
  if (addr == 0xFE20) {
    vula.control_reg = value;
    return;
  }
  u8 logical                = (value >> 4) & 0x0F;
  u8 physical               = (value & 0x0F) ^ 7;
  vula.palette_reg[logical] = physical;
}

u8 vula_read(u16 addr) {
  if (addr == 0xFE20)
    return vula.control_reg;
  return 0xFF; // palette is write-only, 0xFF is correct for those
}

u8 get_pixels_per_byte() {
  switch (vula.control_reg) {
  case 0b10011100:
    return 8; // MODE 0 & 3, 1bpp
  case 0b11011000:
    return 4; // MODE 1, 2bpp
  case 0b11110100:
    return 2; // MODE 2, 4bpp
  case 0b10001000:
    return 8; // MODE 4 & 6, 1bpp
  case 0b11000100:
    return 4; // MODE 5, 2bpp
  default:
    return 8;
  }
}

u8 get_cursor_width() {
  switch (vula.control_reg >> 5) {
  case 0b010:
    return 1; // MODE 7
  case 0b100:
    return 1; // MODE 0, 3, 4, 6
  case 0b110:
    return 2; // MODE 1, 5
  case 0b111:
    return 4; // MODE 2
  default:
    return 0;
  }
}

u8 is_teletext() { return (vula.control_reg >> 1) & 0x1; }

u8 extract_pixel(u8 byte, u8 p, u8 bpp) {
  /*
   * Pixel data is stored in reverse-order, round-robin
   * bpp is bits per pixel
   * 1 bpp:
   *   7 6 5 4 3 2 1 0
   * 2 bpp:
   *   73 62 51 40
   * 4 bpp:
   *   7531 6420
   */

  switch (bpp) {
  case 1:
    return ((byte >> (7 - p)) & 0x1) << 3;
  case 2: {
    u8 hi = (byte >> (7 - p)) & 0x1;
    u8 lo = (byte >> (3 - p)) & 0x1;
    return (hi << 3) | (lo << 1);
  }
  case 4: {
    u8 b3 = (byte >> (7 - p)) & 0x1;
    u8 b2 = (byte >> (5 - p)) & 0x1;
    u8 b1 = (byte >> (3 - p)) & 0x1;
    u8 b0 = (byte >> (1 - p)) & 0x1;
    return (b3 << 3) | (b2 << 2) | (b1 << 1) | b0;
  }
  }
  return 0;
}

void dump_vula() {
  printf("control_reg: %b\n", vula.control_reg);
  for (int i = 0; i < 16; i++) {
    printf("palette %d: %d\n", i, vula.palette_reg[i]);
  }
}
