#include <stddef.h>
#include <stdio.h>

#include "crtc.h"
#include "keyboard.h"
#include "memory.h"
#include "processor.h"
#include "raylib.h"
#include "via.h"
#include "vula.h"

#define H_PADDING 50
#define SCREEN_WIDTH 960
#define SCREEN_HEIGHT 768

void draw_screen() {
  // NOTE: this is currently hard-coded for MODE 4
  int scale = SCREEN_WIDTH / 8 / crtc.c_per_row;

  u16 vram_addr = crtc.upper_left_character_mem_loc * 8;
  for (int row = 0; row < crtc.n_rows; row++) {
    for (int scan = 0; scan <= crtc.scan_lines_per_c; scan++) {
      for (int col = 0; col < crtc.c_per_row; col++) {
        u8 byte =
            mem_read(vram_addr + col * (crtc.scan_lines_per_c + 1) + scan);
        for (int b = 7; b >= 0; b--) {
          int pixel = (byte >> b) & 1;
          DrawRectangle(H_PADDING + scale * (col * 8 + 7 - b),
                        scale * (row * (crtc.scan_lines_per_c + 1) + scan),
                        scale, scale, pixel ? WHITE : BLACK);
        }
      }
    }
    vram_addr += crtc.c_per_row * (crtc.scan_lines_per_c + 1);
  }
}

int main() {
  mem_init();
  cpu_reset();

  InitWindow(H_PADDING * 2 + SCREEN_WIDTH, SCREEN_HEIGHT, "BBCMicro Emulator");

  printf("Starting execution loop...\n");

  for (;;) {
    if (cpu.PC == 0xE539) {
      printf("Entered BASIC keypress loop...\n");
      break;
    }
    cpu_exec();
  }

  SetTargetFPS(300);

  while (!WindowShouldClose()) {
    u8 ch;
    while ((ch = GetCharPressed()) != 0) {
      // inserting key into buffer manually
      write_keyboard_buffer(ch);
      /* printf("%c pressed\n", ch); */

      // for key with override
      /* system_via.ifr |= 1; */
      /* system_via.reg_a = (1 << 7) | ascii_to_bbc_key[pressed]; */
      /* if ((system_via.ier & 1) == 1) */
      /*   cpu_irq(); */
    }

    if (IsKeyPressed(KEY_ENTER)) {
      write_keyboard_buffer(13);
      /* printf("Enter pressed\n"); */
    }

    if (IsKeyPressed(KEY_BACKSPACE)) {
      write_keyboard_buffer(127);
      /* printf("Backspace pressed\n"); */
    }

    BeginDrawing();
    ClearBackground(BLACK);
    draw_screen();
    EndDrawing();

    for (int i = 0; i < 10000; i++) {
      cpu_exec();
      /* printf("PC: 0x%0X (0x%0x)\n", cpu.PC, mem_read(cpu.PC)); */
    }
  }

  CloseWindow();

  return 0;
}
