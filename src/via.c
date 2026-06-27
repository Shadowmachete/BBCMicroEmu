/*
 *  VIA Module
 *
 *  0xFE40-0xFE5F
 *  40: Register B (top 4 bits for reading, bottom 4 for writing)
 *    0   Enable sound chip
 *    1   Enable Read Speech
 *    2   Enable Write Speech
 *    3   Disable Keyboard auto scanning
 *    4   Hardware scrolling - set C0=0 (See below)
 *    5   Hardware scrolling - set C1=0 (See below)
 *    6   Turn on CAPS LOCK LED
 *    7   Turn on SHIFT LOCK LED
 *    8   Disable sound chip
 *    9   Disable Read Speech
 *    10  Disable Write Speech
 *    11  Enable Keyboard auto scanning
 *    12  Hardware scrolling - set C0=1 (See below)
 *    13  Hardware scrolling - set C1=1 (See below)
 *    14  Turn off CAPS LOCK LED
 *    15  Turn off SHIFT LOCK LED
 *    The values of C0 and C1 together determine the start scroll address for
 *      the screen:
 *
 *       C0   C1      Screen       Used in
 *                    Address   Regular MODEs
 *       ------------------------------------
 *        0    0      $4000           3
 *        0    1      $5800          4,5
 *        1    0      $6000           6
 *        1    1      $3000         0,1,2
 *
 *        NOTE: apparently this isnt correct and its 11 for 4,5 and 01 for 0,1,2
 *              for my mos.rom
 *
 *    When reading from this address the top four bits are read:
 *    bit 7:    Speech processor 'ready' signal
 *    bit 6:    Speech processor 'interrupt' signal
 *    bit 4-5:  joystick buttons (bit is zero when button pressed)
 *  41: Register A [unused], use non-handshaking variant
 *  42: DDRB, always set to 0b00001111
 *  43: DDRA
 *    Sound: 0xFF, all bits written are output bits
 *    Speech: 0x00 for reading and 0xFF for writing
 *    Keyboard: 0b7F for reading, key to read is in bits 0-6 of the
 *      data and `pressed` state is in bit 7
 *  44-47: Timer 1 registers
 *    1Mhz countdown, used as a 100Hz timer
 *    One-shot:
 *      LatchLow (46) and CounterHigh (45) make a 16-bit countdown value
 *    Free-run (default mode):
 *      LatchLow and LatchHigh (47) are set as the initial timeout
 *      Once interrupt, reset counter and continue
 *      Stopped by writing CounterHigh, reading CounterLow (44) or writing
 *        to IF
 *  48-49: Timer 2 registers
 *    Used for updating speech system
 *    One-shot:
 *      Same as timer 1
 *    Pulse-counting:
 *      Counts down negative going pulses
 *  4A: Shift register
 *  4B: Auxiliary Control register
 *    bit 0: PA latch enable
 *    bit 1: PB latch enable
 *    bit 2-4: Shift register mode
 *    bit 5: timer 2 mode
 *    bit 6: timer 1 mode
 *    bit 7: enable pulsing of PB7
 *  4C: Peripheral Control register
 *    bit 0: CA1 interrupt control
 *    bit 1-3: CA2 control mode
 *    bit 4: CB1 interrupt control
 *    bit 5-7: CB2 control mode
 *    control mode:
 *      000 = negative edges active on input
 *      001 = independent interrupt; input negative edge
 *      010 = positive edges active on input
 *      011 = independent interrupt; input positive edge
 *      100 = handshake output mode
 *      101 = pulse output mode
 *      110 = low output
 *      111 = high output
 *  4D: Interrupt Flag register
 *    bit 0 = key pressed
 *    bit 1 = vertical sync occurred
 *    bit 2 = shift register timeout (unused)
 *    bit 3 = lightpen strobe off screen
 *    bit 4 = analogue conversion completed
 *    bit 5 = timer 2 timed out (used for speech)
 *    bit 6 = timer 1 timed out (100Hz signal)
 *    bit 7 = (when reading) master interrupt flag
 *  4E: Interrupt Enable register
 *    write with bit 7 set or clear to enable/disable interrupt
 *  4F: Register A without handshaking
 */

#include "via.h"
#include "types.h"
#include <stdio.h>

VIA system_via = {.ier = 0xF};
VIA user_via;

u8 system_via_read(u16 addr) {
  switch (addr & 0x0F) {
  case 0x0: {
    // speech ready, interrupt off, joysticks not pressed
    return 0b10110000 | (system_via.reg_b & 0x0F);
  } break;
  case 0x1: {
    // unused
    return system_via.reg_a;
  } break;
  case 0x2: {
    return system_via.ddrb;
  } break;
  case 0x3: {
    return system_via.ddra;
  } break;
  case 0x4: {
    return system_via.timer1_counterlow;
  } break;
  case 0x5: {
    return system_via.timer1_counterhigh;
  } break;
  case 0x6: {
    return system_via.timer1_latchlow;
  } break;
  case 0x7: {
    return system_via.timer1_latchhigh;
  } break;
  case 0x8: {
    return system_via.timer2_counterlow;
  } break;
  case 0x9: {
    return system_via.timer2_counterhigh;
  } break;
  case 0xA: {
    return system_via.shift_reg;
  } break;
  case 0xB: {
    return system_via.acr;
  } break;
  case 0xC: {
    return system_via.pcr;
  } break;
  case 0xD: {
    u8 pending = system_via.ifr & system_via.ier & 0x7F;
    return pending ? (system_via.ifr | 0x80) : (system_via.ifr & 0x7F);
  } break;
  case 0xE: {
    return system_via.ier;
  } break;
  case 0xF: {
    return system_via.reg_a;
  } break;
  }
  return 0x00;
}

void system_via_write(u16 addr, u8 value) {
  switch (addr & 0x0F) {
  case 0x0: {
    u8 line          = value & 7;
    system_via.reg_b = (system_via.reg_b & ~(1 << line)) | ((value >> 3) & 1)
                                                               << line;
  } break;
  case 0x1: {
    // unused
    system_via.reg_a = value;
  } break;
  case 0x2: {
    // value should be 0b00001111
    system_via.ddrb = value;
  } break;
  case 0x3: {
    system_via.ddra = value;
  } break;
  case 0x4: {
    system_via.timer1_counterlow = value;
  } break;
  case 0x5: {
    system_via.timer1_counterhigh = value;
  } break;
  case 0x6: {
    system_via.timer1_latchlow = value;
  } break;
  case 0x7: {
    system_via.timer1_latchhigh = value;
  } break;
  case 0x8: {
    system_via.timer2_counterlow = value;
  } break;
  case 0x9: {
    system_via.timer2_counterhigh = value;
  } break;
  case 0xA: {
    system_via.shift_reg = value;
  } break;
  case 0xB: {
    system_via.acr = value;
  } break;
  case 0xC: {
    system_via.pcr = value;
  } break;
  case 0xD: {
    system_via.ifr &= ~(value & 0x7F);
  } break;
  case 0xE: {
    b8 enable = (value & 0x80) != 0;
    if (enable) {
      system_via.ier |= value;
    } else {
      system_via.ier &= ~value;
    }
  } break;
  case 0xF: {
    system_via.reg_a = value;
  } break;
  }
}

u16 get_vram_base() {
  // C0 is bit 4 and C1 is bit 5
  switch ((system_via.reg_b >> 4) & 0b11) {
  case 0b00:
    return 0x4000; // MODE 3
  case 0b11:
    return 0x5800; // MODEs 4, 5
  case 0b01:
    return 0x6000; // MODE 6
  case 0b10:
    return 0x3000; // MODEs 0, 1, 2
  }

  return 0x5800;
}

void dump_system_via() {
  printf("reg b: %0b\n", system_via.reg_b);
  printf("reg a: %0b\n", system_via.reg_a);
}
