/**
 *  Memory module for BBC Micro
 *
 *  64KB of memory - 256 x 256B pages
 *  0x0000 - 0x7FFF: 32KB of RAM
 *    0x0000 - 0x00FF: Zero Page
 *    0x0100 - 0x01FF: Stack
 *    0x0200 - 0x7FFF: Free memory
 *  0x8000 - 0xBFFF: 16KB of BASIC ROM
 *  0xC000 - 0xFFFF: 16KB of OS ROM
 *    0xFE00 - 0xFEFF: SHEILA / system hardware
 *      00-07: 6845 CRTC (Video controller)
 *      08-0F: 6850 ACIA (Serial controller)
 *      10-1F: Serial ULA (Serial system chip)
 *      20-2F: Video ULA (Video system chip)
 *      30-3F: 74LS161 (Paged ROM selector)
 *      40-5F: 6522 VIA (SYSTEM VIA)
 *      60-7F: 6522 VIA (USER VIA)
 *      80-9F: 8271 FDC (Floppy disk contoller)
 *      A0-BF: 68B54 ADLC (ECONET controller)
 *      C0-DF: uPD7002 (Analogue to digital converter)
 *      E0-FF: Tube ULA (Tube system interface)
 *    0xFFFA - 0xFFFF: Reserved for resets and interrupts
 *      A-B: non-maskable interrupt handler
 *      C-D: power on reset location
 *      E-F: BRK / interrupt request handler
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "processor.h"
#include "types.h"
#include "via.h"

u8 ram[0x8000]       = {0}; // 32KB of ram + io
u8 os_rom[0x4000]    = {0}; // 16KB of OS rom
u8 basic_rom[0x4000] = {0}; // 16KB of language rom

u8 *page_ptr[PAGE_COUNT];
b8 readonly[PAGE_COUNT] = {0};

io_read_t page_read_handler[PAGE_COUNT];
io_write_t page_write_handler[PAGE_COUNT];

u8 generic_read_handler(u16 addr) {
  printf("Reading from 0x%0X\n", addr);
  (void)addr;
  return 0xFF;
}

u8 fe_read(u16 addr) {
  u8 reg = addr & 0xFF;
  /* if (reg >= 0x00 && reg <= 0x07) */
  /*   return crtc_read(addr); */
  /* if (reg >= 0x08 && reg <= 0x0F) */
  /*   return acia_read(addr); */
  /* if (reg >= 0x10 && reg <= 0x1F) */
  /*   return serial_ula_read(addr); */
  /* if (reg >= 0x20 && reg <= 0x2F) */
  /*   return video_ula_read(addr); */
  /* if (reg >= 0x30 && reg <= 0x3F) */
  /*   return paged_rom_selector_read(addr); */
  if (reg >= 0x40 && reg <= 0x5F)
    return system_via_read(addr);
  /* if (reg >= 0x60 && reg <= 0x7F) */
  /*   return user_via_read(addr); */
  /* if (reg >= 0x80 && reg <= 0x9F) */
  /*   return fdc_read(addr); */
  /* if (reg >= 0xA0 && reg <= 0xBF) */
  /*   return adlc_read(addr); */
  /* if (reg >= 0xC0 && reg <= 0xDF) */
  /*   return upd_read(addr); */
  /* if (reg >= 0xE0 && reg <= 0xFF) */
  /*   return tube_ula_read(addr); */

  return 0xFF;
}

void noop_write_handler(u16 addr, u8 value) {
  (void)addr;
  (void)value;
}

void fe_write(u16 addr, u8 value) {
  u8 reg = addr & 0xFF;
  /* if (reg >= 0x00 && reg <= 0x07) */
  /*   crtc_write(addr, value); */
  /* if (reg >= 0x08 && reg <= 0x0F) */
  /*   acia_write(addr, value); */
  /* if (reg >= 0x10 && reg <= 0x1F) */
  /*   serial_ula_write(addr, value); */
  /* if (reg >= 0x20 && reg <= 0x2F) */
  /*   video_ula_write(addr, value); */
  /* if (reg >= 0x30 && reg <= 0x3F) */
  /*   paged_rom_selector_write(addr, value); */
  if (reg >= 0x40 && reg <= 0x5F)
    system_via_write(addr, value);
  /* if (reg >= 0x60 && reg <= 0x7F) */
  /*   user_via_write(addr, value); */
  /* if (reg >= 0x80 && reg <= 0x9F) */
  /*   fdc_write(addr, value); */
  /* if (reg >= 0xA0 && reg <= 0xBF) */
  /*   adlc_write(addr, value); */
  /* if (reg >= 0xC0 && reg <= 0xDF) */
  /*   upd_write(addr, value); */
  /* if (reg >= 0xE0 && reg <= 0xFF) */
  /*   tube_ula_write(addr, value); */
}

void map_page(int page_index, u8 *backing) { page_ptr[page_index] = backing; }

void map_page_io(int page_index, io_read_t r, io_write_t w) {
  readonly[page_index]           = 0;
  page_read_handler[page_index]  = r;
  page_write_handler[page_index] = w;
}

void mem_init(void) {
  printf("Initialising memory...\n");

  memset(ram, 0, sizeof(ram));
  memset(readonly, 0, sizeof(readonly));

  for (int i = 0; i < PAGE_COUNT; ++i) {
    page_ptr[i]           = NULL;
    page_read_handler[i]  = NULL;
    page_write_handler[i] = NULL;
  }

  // Load BASIC ROM from disk
  FILE *f = fopen("basic.rom", "rb");

  if (f == NULL) {
    perror("Failed to open basic.rom file\n");
    exit(1);
  }

  size_t n = fread(basic_rom, 1, ROM_SIZE, f);
  fclose(f);

  if (n != ROM_SIZE) {
    perror("Failed to read all of basic.rom file\n");
    exit(1);
  }

  // Load OS ROM from disk
  f = fopen("mos.rom", "rb");

  if (f == NULL) {
    perror("Failed to open mos.rom file\n");
    exit(1);
  }

  n = fread(os_rom, 1, ROM_SIZE, f);
  fclose(f);

  if (n != ROM_SIZE) {
    perror("Failed to read all of os.rom file\n");
    exit(1);
  }

  printf("OS and BASIC ROM loaded\n");
  printf("Mapping memory regions...\n");

  // map RAM
  for (int p = 0x00; p <= 0x7F; ++p) {
    map_page(p, &ram[p * 256]);
    readonly[p] = 0;
    if (page_read_handler[p])
      page_read_handler[p] = NULL;

    if (page_write_handler[p])
      page_write_handler[p] = NULL;
  }

  // map BASIC ROM
  for (int p = 0x80; p <= 0xBF; ++p) {
    map_page(p, &basic_rom[(p - 0x80) * 256]);
    readonly[p] = 1;
  }

  // map OS ROM
  for (int p = 0xC0; p <= 0xFF; ++p) {
    map_page(p, &os_rom[(p - 0xC0) * 256]);
    readonly[p] = 1;
  }

  // not using fred and wilma memory yet
  map_page_io(0xFC, *generic_read_handler, *noop_write_handler);
  map_page_io(0xFD, *generic_read_handler, *noop_write_handler);
  map_page_io(0xFE, *fe_read, *fe_write);

  printf("RAM and ROM mapped\n");
}

u8 mem_read(u16 addr) {
  u8 p = (addr >> 8) & 0xFF;
  if (page_read_handler[p]) {
    printf("Reading from 0x%0X\n", addr);
    return page_read_handler[p](addr);
  }
  u8 *base = page_ptr[p];
  if (base)
    return base[addr & 0xFF];

  printf("Attempt to read from unmapped memory address 0x%04X ignored\n", addr);

  return 0xFF; // unmapped memory
}

void mem_write(u16 addr, u8 v) {
  u8 p = (addr >> 8) & 0xFF;
  if (readonly[p]) {
    printf("Attempt to write to read-only memory address 0x%04X denied\n",
           addr);
    return;
  }

  if (page_write_handler[p]) {
    printf("Writing 0x%0X to 0x%0X\n", v, addr);
    page_write_handler[p](addr, v);
    return;
  }

  u8 *base = page_ptr[p];
  if (base) {
    base[addr & 0xFF] = v;
    return;
  }

  printf("Attempt to write to unmapped memory address 0x%04X ignored\n", addr);

  return; // unmapped memory
}

void stack_push(u8 v) {
  /* if (cpu.SP == 0x00) { */
  /*   printf("Attempt to push onto full stack ignored\n"); */
  /*   return; */
  /* } */

  mem_write(cpu.SP | 0x0100, v);
  cpu.SP--;
}

void stack_push_16(u16 v) {
  /* if (cpu.SP == 0x00) { */
  /*   printf("Attempt to push onto full stack ignored\n"); */
  /*   return; */
  /* } */

  // little endian so
  // stack[sp] = high, stack[sp--] = low

  u8 high = (v >> 8) & 0xFF;
  u8 low  = v & 0xFF;

  stack_push(high);
  stack_push(low);
}

u8 stack_pull() {
  /* if (cpu.SP == 0xFF) { */
  /*   printf("Attempt to pull from empty stack ignored\n"); */
  /*   return 0; */
  /* } */

  ++cpu.SP;
  return mem_read(cpu.SP | 0x0100);
}

u16 stack_pull_16() {
  /* if (cpu.SP == 0xFF) { */
  /*   printf("Attempt to pull from empty stack ignored\n"); */
  /*   return 0; */
  /* } */

  u8 low  = stack_pull();
  u8 high = stack_pull();

  return ((u16)high << 8) | low;
}
