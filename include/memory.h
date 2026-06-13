#ifndef MEMORY_H
#define MEMORY_H

#include "types.h"

#define PAGE_COUNT 256
#define PAGE_SIZE 256
#define MEM_SIZE 0x10000
#define RAM_SIZE 0x8000
#define ROM_SIZE 0x4000

#define READ_16LE(addr) (mem_read((addr) + 1) << 8) + (mem_read((addr)))
#define WRITE_16LE(addr, val)                                                  \
  mem_write((addr) + 1, (val >> 8) & 0xFF);                                    \
  mem_write((addr), val & 0xFF);

extern u8 ram[RAM_SIZE];       // 32KB of ram + io
extern u8 os_rom[ROM_SIZE];    // 16KB of OS rom
extern u8 basic_rom[ROM_SIZE]; // 16KB of language rom

typedef u8 (*io_read_t)(u16 addr);
typedef void (*io_write_t)(u16 addr, u8 value);

extern u8 *page_ptr[PAGE_COUNT];

extern io_read_t page_read_handler[PAGE_COUNT];
extern io_write_t page_write_handler[PAGE_COUNT];

void map_page(int page_index, u8 *backing);
void map_page_io(int page_index, io_read_t r, io_write_t w);

void mem_init(void);

u8 mem_read(u16 addr);          /* read a byte from memory */
void mem_write(u16 addr, u8 v); /* write a byte to memory */

void stack_push(u8 v);
void stack_push_16(u16 v);

u8 stack_pull();
u16 stack_pull_16();

void dump_vram();

#endif // !MEMORY_H
