/*
 *  System Via Module
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
 *
 *  41: Register A [unused], use non-handshaking variant
 *  42: DDRB, always set to 0b00001111
 *  43: DDRA
 *    Sound: 0xFF, all bits written are output bits
 *    Speech: 0x00 for reading and 0xFF for writing
 *    Keyboard: 0b01111111 for reading, key to read is in bits 0-6 of the
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
 */

#include "via.h"
#include "types.h"

u8 system_via_read(u16 addr) {
  (void)addr;
  return 0x00;
}

void system_via_write(u16 addr, u8 value) {
  (void)addr;
  (void)value;
}
