/*
 *  FDC Module
 *
 *  https://pulsar-cad.com/datasheets/54/05/00000000554.pdf
 *
 *  0xFE80-0xFE9F
 *  80: Read status register, write control register
 *  81: Read result register, write parameter register
 *  82: Write reset register
 *  83: Not used
 *  84: Read and write data
 *
 *  Command Register
 *    bits 0-5: command opcode
 *    bits 6-7: surface / drive (SELECT 0, 1)
 *  Parameter Register
 *    expected parameter
 *  Result Register
 *    bit 0: unused, 0
 *    bits 1-2: completion code
 *    bits 3-4: completion type
 *      type and code
 *
 *      good completion - no error
 *      00:00 - good completion / scan not met
 *      00:01 - scan met equal
 *      00:10 - scan met not equal
 *      00:11 - ---
 *
 *      system error - recoverable error
 *      01:00 - clock error
 *      01:01 - late dma
 *      01:10 - id crc error
 *      01:11 - data crc error
 *
 *      operator intervention required for recovery
 *      10:00 - drive not ready
 *      10:01 - write protect
 *      10:10 - track 0 not found
 *      10:11 - write fault
 *
 *      command error - program error or drive hardware failure
 *      11:00 - sector not found
 *      rest unused
 *
 *    bit 5: deleted data found
 *    bits 6-7: unused, 00
 *  Status Register
 *    bits 0-1: unused, 00
 *    bit 2: non-dma data request
 *    bit 3: interrupt request
 *    bit 4: result register full
 *    bit 5: parameter register full
 *    bit 6: command register full
 *    bit 7: command busy
 *  Reset Register
 *    reset the 8271
 *
 *  TODO: Interrupt line and DMA operations (if needed)
 */

#include <stdio.h>
#include <stdlib.h>

#include "fdc.h"
#include "processor.h"

#define SECTORS_PER_TRACK 10
#define SECTOR_SIZE 256

FDC fdc     = {0};
u8 *ssd_buf = NULL;

// executes the command after receiving all parameters
void fdc_exec_command();

void fdc_reset() {
  printf("reset fdc\n");

  if (fdc.param_buf != NULL)
    free(fdc.param_buf);

  fdc = (FDC){0};

  /* fdc.sp_reg = (FDC_SP_REG){0}; */
  /* fdc.cmd_reg     = 0; */
  /* fdc.param_buf   = (void *)0; */
  /* fdc.param_count = 0; */
  /* fdc.param_idx   = 0; */
  /* fdc.res_reg     = 0; */
  /* fdc.status_reg  = 0; */
  /* fdc.reset_ready = 0; */
  /* fdc.head = 0; */
}

u8 fdc_read(u16 addr) {
  switch (addr & 0xFF) {
  case 0x80: {
    /* printf("read stat reg\n"); */
    return fdc.status_reg;
  }
  case 0x81: {
    printf("read res reg %d\n", fdc.res_reg);
    fdc.status_reg &= ~(1 << 3); // clear interrupt request
    fdc.status_reg &= ~(1 << 4); // result empty
    return fdc.res_reg;
  }
  case 0x84: {
    /* printf("read data\n"); */

    u8 val = 0x00;

    fdc.status_reg &= ~(1 << 2); // clear non-dma data request

    if (fdc.bytes_remaining) {
      val = ssd_buf[fdc.data_offset];
      fdc.data_offset++;
      fdc.bytes_remaining--;
    }

    if (fdc.bytes_remaining) {
      // stream next byte
      fdc.status_reg |= 1 << 2; // set non-dma data request
    } else {
      // result phase
      fdc.param_idx   = 0;
      fdc.param_count = 0;
      fdc.cmd_reg     = 0;
      fdc.status_reg |= 1 << 3;    // set interrupt request
      fdc.status_reg |= 1 << 4;    // result full
      fdc.status_reg &= ~(1 << 6); // command empty
      fdc.status_reg &= ~(1 << 7); // command free
    }

    cpu.nmi_pending = 1;
    fdc.res_reg     = 0; // result ok

    return val;
  }
  }

  return 0x00;
}

void fdc_write(u16 addr, u8 v) {
  switch (addr & 0xFF) {
  case 0x80: {
    printf("set command = 0x%0X\n", v);
    fdc.status_reg |= 1 << 7; // command busy
    fdc.status_reg |= 1 << 6; // command full
    fdc.cmd_reg = v;

    // save parameter count
    switch (v & 0x3F) {
    case SCAN:
    case SCAN_DEL: {
      fdc.param_count = 5;
    } break;
    case WRITE:
    case WRITE_DEL:
    case READ:
    case READ_DEL:
    case VERIFY_DEL: {
      fdc.param_count = 2;
    } break;
    case WRITE_MULTI:
    case WRITE_DEL_MULTI:
    case READ_MULTI:
    case READ_DEL_MULTI:
    case VERIFY_DEL_MULTI: {
      fdc.param_count = 3;
    } break;
    case READ_ID: {
      fdc.param_count = 3;
    } break;
    case FORMAT: {
      fdc.param_count = 5;
    } break;
    case SEEK: {
      fdc.param_count = 1;
    } break;
    case READ_STATUS: {
      fdc.param_count = 0;
    } break;
    case SPECIFY: {
      fdc.param_count = 4;
    } break;
    case WRITE_SP_REG: {
      fdc.param_count = 2;
    } break;
    case READ_SP_REG: {
      fdc.param_count = 1;
    } break;
    }

    if (fdc.param_count == 0) {
      fdc_exec_command();
      break;
    }

    fdc.param_buf = (u8 *)realloc(fdc.param_buf, fdc.param_count * sizeof(u8));
  } break;
  case 0x81: {
    printf("set param = 0x%0X\n", v);

    if (fdc.param_idx == fdc.param_count) {
      printf("Received all %d parameters, did not expect to get another\n",
             fdc.param_count);
      return;
    }

    fdc.param_buf[fdc.param_idx++] = v;

    if (fdc.param_idx == fdc.param_count)
      fdc_exec_command();
  } break;
  case 0x82: {
    if (v == 1) {
      fdc.reset_ready = 1;
      return;
    }

    if (v == 0 && fdc.reset_ready)
      fdc_reset();

    fdc.reset_ready = 0;
  } break;
  case 0x84: {
    printf("write 0x%0X to data\n", v);
    // fdc.data = v;
  } break;
  }
}

void fdc_load_ssd(const char *file_name) {
  FILE *f = fopen(file_name, "rb");

  if (f == NULL) {
    fprintf(stderr, "Failed to open file %s\n", file_name);
    exit(1);
  }

  fseek(f, 0L, SEEK_END);
  u8 file_size = ftell(f);
  fseek(f, 0L, SEEK_SET);

  ssd_buf = realloc(ssd_buf, file_size);

  size_t n = fread(ssd_buf, 1, file_size, f);
  fclose(f);

  if (n != file_size) {
    fprintf(stderr, "Failed to read all of %s\n", file_name);
    exit(1);
  }

  printf("Loaded %s\n", file_name);
}

u16 get_record_length() {
  switch ((fdc.param_buf[2] >> 5) & 0x7) {
  case 0b000:
    return 128;
  case 0b001:
    return 256;
  case 0b010:
    return 512;
  case 0b011:
    return 1024;
  case 0b100:
    return 2048;
  case 0b101:
    return 4096;
  case 0b110:
    return 8192;
  case 0b111:
    return 16384;
  }

  return 0;
}

void fdc_exec_command() {
  switch (fdc.cmd_reg & 0x3F) {
  case SCAN: {
  } break;
  case SCAN_DEL: {
  } break;
  case WRITE: {
  } break;
  case WRITE_MULTI: {
  } break;
  case WRITE_DEL: {
  } break;
  case WRITE_DEL_MULTI: {
  } break;
  case READ: {
  } break;
  case READ_MULTI: {
    fdc.bytes_remaining = get_record_length() * (fdc.param_buf[2] & 0x1F);
    fdc.data_offset =
        (fdc.param_buf[0] * SECTORS_PER_TRACK + fdc.param_buf[1]) * SECTOR_SIZE;

    fdc.res_reg = 0;          // result ok
    fdc.status_reg |= 1 << 2; // set non-dma data request
    cpu.nmi_pending = 1;
    return;
  } break;
  case READ_DEL: {
  } break;
  case READ_DEL_MULTI: {
  } break;
  case READ_ID: {
    // params: track address, 0, number of id fields
  } break;
  case VERIFY_DEL: {
  } break;
  case VERIFY_DEL_MULTI: {
  } break;
  case FORMAT: {
    // pararmeters in order 0 to 4
    // track address, gap 3 size - 6, record length | no sectors / track,
    // gap 5 size - 6, gap 1 size - 6
    // possibly unused
  } break;
  case SEEK: {
    // move curr track to corresponding track
    fdc.sp_reg.s0_curr_track = fdc.param_buf[0];
    fdc.res_reg              = 0;
  } break;
  case READ_STATUS: {
    // put final drive status in result register
    // bit 7 to 0
    // --, rdy 1, wr fault, index, wr prot, rdy 0, track 0, cnt / op
    fdc.res_reg = 0b00000110;
    fdc.status_reg |= 1 << 4; // result full
  } break;
  case SPECIFY: {
    // TODO: load bad tracks
    // no bad tracks since .ssd is defect free
    if (fdc.param_buf[0] == 0x10) {
      // surface 0 bad tracks
      fdc.sp_reg.s0_bad1       = fdc.param_buf[1];
      fdc.sp_reg.s0_bad2       = fdc.param_buf[2];
      fdc.sp_reg.s0_curr_track = fdc.param_buf[3];
      break;
    }

    if (fdc.param_buf[0] == 0x18) {
      // surface 1 bad tracks
      fdc.sp_reg.s1_bad1       = fdc.param_buf[1];
      fdc.sp_reg.s1_bad2       = fdc.param_buf[2];
      fdc.sp_reg.s1_curr_track = fdc.param_buf[3];
      break;
    }

    // initialise timings
    u8 step_rate          = fdc.param_buf[1];
    u8 head_settling_time = fdc.param_buf[2];
    u8 index_count        = (fdc.param_buf[3] >> 4) & 0xF;
    u8 head_load_time     = fdc.param_buf[3] & 0xF;

    // unused i guess?
    (void)step_rate;
    (void)head_settling_time;
    (void)index_count;
    (void)head_load_time;
  } break;
  case WRITE_SP_REG: {
    u8 v = fdc.param_buf[1];
    switch (fdc.param_buf[0]) {
    case SCAN_SEC_NUM: {
      fdc.sp_reg.scan_sec_num = v;
    } break;
    case SCAN_MSB_COUNT: {
      fdc.sp_reg.scan_msb_count = v;
    } break;
    case SCAN_LSB_COUNT: {
      fdc.sp_reg.scan_lsb_count = v;
    } break;
    case S0_CURR_TRACK: {
      fdc.sp_reg.s0_curr_track = v;
    } break;
    case S1_CURR_TRACK: {
      fdc.sp_reg.s1_curr_track = v;
    } break;
    case MODE_REGISTER: {
      // non-dma is 0b11000001
      // confirmed that this value is set by dfs at init
      fdc.sp_reg.mode_register = v;
    } break;
    case DRIVE_CTL_OUT: {
      // format is (from bit 7 to bit 0)
      // select 1, select 0, write fault reset / optional output,
      // low head current, load head, direction, seek / step, write enable
      fdc.sp_reg.drive_ctl_out = v;
    } break;
    case DRIVE_CTL_IN: {
      fdc.sp_reg.drive_ctl_in = v;
    } break;
    case S0_BAD1: {
      fdc.sp_reg.s0_bad1 = v;
    } break;
    case S0_BAD2: {
      fdc.sp_reg.s0_bad2 = v;
    } break;
    case S1_BAD1: {
      fdc.sp_reg.s1_bad1 = v;
    } break;
    case S1_BAD2: {
      fdc.sp_reg.s1_bad2 = v;
    } break;
    }
  } break;
  case READ_SP_REG: {
    fdc.status_reg |= 1 << 4; // result full
    switch (fdc.param_buf[0]) {
    case SCAN_SEC_NUM: {
      fdc.res_reg = fdc.sp_reg.scan_sec_num;
    } break;
    case SCAN_MSB_COUNT: {
      fdc.res_reg = fdc.sp_reg.scan_msb_count;
    } break;
    case SCAN_LSB_COUNT: {
      fdc.res_reg = fdc.sp_reg.scan_lsb_count;
    } break;
    case S0_CURR_TRACK: {
      fdc.res_reg = fdc.sp_reg.s0_curr_track;
    } break;
    case S1_CURR_TRACK: {
      fdc.res_reg = fdc.sp_reg.s1_curr_track;
    } break;
    case MODE_REGISTER: {
      fdc.res_reg = fdc.sp_reg.mode_register;
    } break;
    case DRIVE_CTL_OUT: {
      fdc.res_reg = fdc.sp_reg.drive_ctl_out;
    } break;
    case DRIVE_CTL_IN: {
      fdc.res_reg = fdc.sp_reg.drive_ctl_in;
    } break;
    case S0_BAD1: {
      fdc.res_reg = fdc.sp_reg.s0_bad1;
    } break;
    case S0_BAD2: {
      fdc.res_reg = fdc.sp_reg.s0_bad2;
    } break;
    case S1_BAD1: {
      fdc.res_reg = fdc.sp_reg.s1_bad1;
    } break;
    case S1_BAD2: {
      fdc.res_reg = fdc.sp_reg.s1_bad2;
    } break;
    }
  } break;
  }

  fdc.param_idx   = 0;
  fdc.param_count = 0;
  fdc.cmd_reg     = 0;
  fdc.status_reg &= ~(1 << 6); // command empty
  fdc.status_reg &= ~(1 << 7); // command free
}

void fdc_cleanup() {
  if (fdc.param_buf != NULL)
    free(fdc.param_buf);

  if (ssd_buf != NULL)
    free(ssd_buf);
}
