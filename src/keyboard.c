#include <stdio.h>

#include "keyboard.h"
#include "memory.h"
#include "types.h"

void write_keyboard_buffer(u8 key) {
  // NOTE: head and tail store buffer positions as offset from 0x0300
  // very strange. so i subtract 0xE0 to get the offset from 0x03E0
  u8 head = mem_read(0x02D8) - 0xE0;
  u8 tail = mem_read(0x02E1) - 0xE0;

  if (((tail + 1) & 0x1F) == head) {
    printf("Buffer is full, cannot write pressed character.\n");
    return;
  }

  mem_write(0x03E0 + tail, key);
  mem_write(0x02E1, ((tail + 1) & 0x1F) + 0xE0);
}
