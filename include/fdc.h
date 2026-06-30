#ifndef FDC_H
#define FDC_H

#include "types.h"

typedef enum {
  SCAN_SEC_NUM   = 0x06,
  SCAN_MSB_COUNT = 0x14,
  SCAN_LSB_COUNT = 0x13,
  S0_CURR_TRACK  = 0x12,
  S1_CURR_TRACK  = 0x1A,
  MODE_REGISTER  = 0x17,
  DRIVE_CTL_OUT  = 0x23,
  DRIVE_CTL_IN   = 0x22,
  S0_BAD1        = 0x10,
  S0_BAD2        = 0x11,
  S1_BAD1        = 0x18,
  S1_BAD2        = 0x19,
} FDC_SP_REG_ADDR;

typedef struct {
  u8 scan_sec_num;
  u8 scan_msb_count;
  u8 scan_lsb_count;
  u8 s0_curr_track;
  u8 s1_curr_track;
  u8 mode_register;
  u8 drive_ctl_out;
  u8 drive_ctl_in;
  u8 s0_bad1;
  u8 s0_bad2;
  u8 s1_bad1;
  u8 s1_bad2;
} FDC_SP_REG;

typedef struct {
  FDC_SP_REG sp_reg;

  u8 cmd_reg;
  u8 param_count;
  u8 *param_buf;
  u8 param_idx;
  u8 res_reg;
  u8 status_reg;

  b8 reset_ready; // for tracking whether a 1 and then 0 is received to cause a
                  // reset

  u32 data_offset;
  u32 bytes_remaining;

  u32 nmi_delay; // instructions until the next data/result NMI is raised
} FDC;

typedef enum {
  SCAN             = 0x00,
  SCAN_DEL         = 0x04,
  WRITE            = 0x0A,
  WRITE_MULTI      = 0x0B,
  WRITE_DEL        = 0x0E,
  WRITE_DEL_MULTI  = 0x0F,
  READ             = 0x12,
  READ_MULTI       = 0x13,
  READ_DEL         = 0x16,
  READ_DEL_MULTI   = 0x17,
  READ_ID          = 0x1B,
  VERIFY_DEL       = 0x1E,
  VERIFY_DEL_MULTI = 0x1F,
  FORMAT           = 0x23,
  SEEK             = 0x29,
  READ_STATUS      = 0x2C,
  SPECIFY          = 0x35,
  WRITE_SP_REG     = 0x3A,
  READ_SP_REG      = 0x3D,
} Opcode;

u8 fdc_read(u16 addr);
void fdc_write(u16 addr, u8 v);

void fdc_tick(void);

void fdc_load_ssd(const char *file_name);

void fdc_cleanup();

#endif // !FDC_H
