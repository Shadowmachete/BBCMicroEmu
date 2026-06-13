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

void dump_vula() {
  printf("control_reg: %d\n", vula.control_reg);
  for (int i = 0; i < 16; i++) {
    printf("palette %d: %d\n", i, vula.palette_reg[i]);
  }
}
