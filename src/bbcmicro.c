#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "crtc.h"
#include "fdc.h"
#include "keyboard.h"
#include "memory.h"
#include "processor.h"
#include "raylib.h"
#include "via.h"
#include "vula.h"

#define H_PADDING 50
#define V_PADDING 30
#define SCREEN_WIDTH 960
#define SCREEN_HEIGHT 768

Color palette_id_to_color(u8 id) {
  switch (id) {
  case 0:
    return BLACK;
  case 1:
    return RED;
  case 2:
    return GREEN;
  case 3:
    return YELLOW;
  case 4:
    return BLUE;
  case 5:
    return MAGENTA;
  case 6:
    return (Color){0, 255, 255, 255};
  case 7:
    return WHITE;
  }

  return BLACK;
}

void draw_cursor(u8 h_scale, u8 v_scale, u16 frame_count) {
  if (crtc.n_rows == 0 || crtc.c_per_row == 0 ||
      (vula.control_reg >> 5) == 0x000)
    return;

  u8 blink_enabled = (crtc.cursor_start >> 6) & 0x1;
  u8 blink_slow    = (crtc.cursor_start >> 5) & 0x1;
  if (blink_enabled) {
    u16 period = blink_slow ? 32 : 16;
    if ((frame_count / period) % 2 == 1)
      return; // hide on alternate periods
  }
  i16 char_offset =
      (i16)(crtc.curr_cursor) - (i16)(crtc.upper_left_character_mem_loc);

  u16 total_chars = crtc.n_rows * crtc.c_per_row;
  char_offset     = ((char_offset % total_chars) + total_chars) % total_chars;
  u16 row         = char_offset / crtc.c_per_row;
  u16 col         = char_offset % crtc.c_per_row;

  u8 scan_start = crtc.cursor_start & 0x1F;
  u8 scan_end   = crtc.cursor_end;

  u16 x = H_PADDING + h_scale * (col * 8);
  u16 y =
      V_PADDING + v_scale * (row * (crtc.scan_lines_per_c + 1) + scan_start);

  u8 height = h_scale * (scan_end - scan_start + 1);
  u8 width  = v_scale * 8 * get_cursor_width();

  DrawRectangle(x, y, width, height, WHITE);
}

void draw_screen(u8 h_scale, u8 v_scale, u16 frame_count) {
  u8 ppb = get_pixels_per_byte();
  u8 bpp = 8 / ppb;

  u16 vram_addr = crtc.upper_left_character_mem_loc * 8;
  u16 vram_base = get_vram_base();
  u16 vram_size = 0x8000 - vram_base;

  if (is_teletext()) {
    return;
  }

  for (int row = 0; row < crtc.n_rows; row++) {
    for (int scan = 0; scan <= crtc.scan_lines_per_c; scan++) {
      if (scan > 7)
        continue;
      for (int col = 0; col < crtc.c_per_row; col++) {
        u16 addr =
            vram_base + (vram_addr - vram_base + col * 8 + scan) % vram_size;
        u8 byte = mem_read(addr);
        for (int p = 0; p < ppb; p++) {
          u8 palette_idx = extract_pixel(byte, p, bpp);

          Color color;
          // for flashing colours
          if (vula.palette_reg[palette_idx] > 7 && (frame_count / 25) % 2 == 1)
            color = palette_id_to_color(15 - vula.palette_reg[palette_idx]);
          else
            color = palette_id_to_color(vula.palette_reg[palette_idx] % 8);

          // TODO: do we need to offset by v_sync_pos and h_sync_pos for
          // realism?

          DrawRectangle(
              H_PADDING + h_scale * (col * ppb + p) * bpp,
              V_PADDING + v_scale * (row * (crtc.scan_lines_per_c + 1) + scan),
              h_scale * bpp, v_scale, color);
        }
      }
    }
    vram_addr += crtc.c_per_row * 8;
  }
}

int main() {
  mem_init();
  cpu_reset();
  fdc_load_ssd("Disc002-ChuckieEgg.ssd");

  InitWindow(H_PADDING * 2 + SCREEN_WIDTH, V_PADDING * 2 + SCREEN_HEIGHT,
             "BBCMicro Emulator");

  printf("Starting execution loop...\n");

  /* for (;;) { */
  /*   if (cpu.PC == 0xE539) { */
  /*     printf("Entered BASIC keypress loop...\n"); */
  /*     break; */
  /*   } */
  /*   printf("PC = 0x%0X\n", cpu.PC); */
  /*   cpu_exec(); */
  /* } */

  SetTargetFPS(60);

  f32 elapsed = 0.0f;

  while (!WindowShouldClose()) {
    elapsed += GetFrameTime();
    u8 ch;
    while ((ch = GetCharPressed()) != 0) {
      // inserting key into buffer manually
      write_keyboard_buffer(ch);

      // for key with override
      /* system_via.ifr |= 1; */
      /* system_via.reg_a = (1 << 7) | ascii_to_bbc_key[pressed]; */
      /* if ((system_via.ier & 1) == 1) */
      /*   cpu_irq(); */
    }

    if (IsKeyPressed(KEY_ENTER))
      write_keyboard_buffer(13);

    if (IsKeyPressed(KEY_BACKSPACE))
      write_keyboard_buffer(127);

    if (IsKeyPressed(KEY_LEFT_ALT)) {
      dump_crtc();
      dump_vula();
      dump_system_via();
    }

    BeginDrawing();
    ClearBackground(BLACK);
    if (crtc.c_per_row != 0 && crtc.n_rows != 0 && crtc.scan_lines_per_c != 0) {
      u8 h_scale = SCREEN_WIDTH / (8 * crtc.c_per_row);
      u8 v_scale = SCREEN_HEIGHT / (crtc.n_rows * (crtc.scan_lines_per_c + 1));
      draw_screen(h_scale, v_scale, (u16)(elapsed * 50));
      draw_cursor(h_scale, v_scale, (u16)(elapsed * 50));
    }
    EndDrawing();

    for (int i = 0; i < 10000; i++) {
      /* printf("PC = 0x%0X\n", cpu.PC); */
      cpu_exec();
    }
  }

  CloseWindow();

  fdc_cleanup();

  return 0;
}
