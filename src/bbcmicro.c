#include <stddef.h>
#include <stdio.h>

#include "memory.h"
#include "processor.h"

int main() {

  mem_init();
  cpu_reset();

  printf("Starting execution loop...\n");

  for (;;) {
    /* printf("PC: 0x%0X\n", cpu.PC); */
    /* printf("0x%0X\n", mem_read(cpu.PC)); */
    cpu_exec();
  }

  return 0;
}
