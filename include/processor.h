#ifndef PROCESSOR_H
#define PROCESSOR_H

#include "types.h"

typedef enum {
  ADC_I   = 0x69,
  ADC_Z   = 0x65,
  ADC_ZX  = 0x75,
  ADC_A   = 0x6D,
  ADC_AX  = 0x7D,
  ADC_AY  = 0x79,
  ADC_IX  = 0x61,
  ADC_IY  = 0x71,
  AND_I   = 0x29,
  AND_Z   = 0x25,
  AND_ZX  = 0x35,
  AND_A   = 0x2D,
  AND_AX  = 0x3D,
  AND_AY  = 0x39,
  AND_IX  = 0x21,
  AND_IY  = 0x31,
  ASL_ACC = 0x0A,
  ASL_Z   = 0x06,
  ASL_ZX  = 0x16,
  ASL_A   = 0x0E,
  ASL_AX  = 0x1E,
  BCC     = 0x90,
  BCS     = 0xB0,
  BEQ     = 0xF0,
  BIT_Z   = 0x24,
  BIT_A   = 0x2C,
  BMI     = 0x30,
  BNE     = 0xD0,
  BPL     = 0x10,
  BRK     = 0x00,
  BVC     = 0x50,
  BVS     = 0x70,
  CLC     = 0x18,
  CLD     = 0xD8,
  CLI     = 0x58,
  CLV     = 0xB8,
  CMP_I   = 0xC9,
  CMP_Z   = 0xC5,
  CMP_ZX  = 0xD5,
  CMP_A   = 0xCD,
  CMP_AX  = 0xDD,
  CMP_AY  = 0xD9,
  CMP_IX  = 0xC1,
  CMP_IY  = 0xD1,
  CPX_I   = 0xE0,
  CPX_Z   = 0xE4,
  CPX_A   = 0xEC,
  CPY_I   = 0xC0,
  CPY_Z   = 0xC4,
  CPY_A   = 0xCC,
  DEC_Z   = 0xC6,
  DEC_ZX  = 0xD6,
  DEC_A   = 0xCE,
  DEC_AX  = 0xDE,
  DEX     = 0xCA,
  DEY     = 0x88,
  EOR_I   = 0x49,
  EOR_Z   = 0x45,
  EOR_ZX  = 0x55,
  EOR_A   = 0x4D,
  EOR_AX  = 0x5D,
  EOR_AY  = 0x59,
  EOR_IX  = 0x41,
  EOR_IY  = 0x51,
  INC_Z   = 0xE6,
  INC_ZX  = 0xF6,
  INC_A   = 0xEE,
  INC_AX  = 0xFE,
  INX     = 0xE8,
  INY     = 0xC8,
  JMP_A   = 0x4C,
  JMP_IND = 0x6C,
  JSR     = 0x20,
  LDA_I   = 0xA9,
  LDA_Z   = 0xA5,
  LDA_ZX  = 0xB5,
  LDA_A   = 0xAD,
  LDA_AX  = 0xBD,
  LDA_AY  = 0xB9,
  LDA_IX  = 0xA1,
  LDA_IY  = 0xB1,
  LDX_I   = 0xA2,
  LDX_Z   = 0xA6,
  LDX_ZY  = 0xB6,
  LDX_A   = 0xAE,
  LDX_AY  = 0xBE,
  LDY_I   = 0xA0,
  LDY_Z   = 0xA4,
  LDY_ZX  = 0xB4,
  LDY_A   = 0xAC,
  LDY_AX  = 0xBC,
  LSR_ACC = 0x4A,
  LSR_Z   = 0x46,
  LSR_ZX  = 0x56,
  LSR_A   = 0x4E,
  LSR_AX  = 0x5E,
  NOP     = 0xEA,
  ORA_I   = 0x09,
  ORA_Z   = 0x05,
  ORA_ZX  = 0x15,
  ORA_A   = 0x0D,
  ORA_AX  = 0x1D,
  ORA_AY  = 0x19,
  ORA_IX  = 0x01,
  ORA_IY  = 0x11,
  PHA     = 0x48,
  PHP     = 0x08,
  PLA     = 0x68,
  PLP     = 0x28,
  ROL_ACC = 0x2A,
  ROL_Z   = 0x26,
  ROL_ZX  = 0x36,
  ROL_A   = 0x2E,
  ROL_AX  = 0x3E,
  ROR_ACC = 0x6A,
  ROR_Z   = 0x66,
  ROR_ZX  = 0x76,
  ROR_A   = 0x6E,
  ROR_AX  = 0x7E,
  RTI     = 0x40,
  RTS     = 0x60,
  SBC_I   = 0xE9,
  SBC_Z   = 0xE5,
  SBC_ZX  = 0xF5,
  SBC_A   = 0xED,
  SBC_AX  = 0xFD,
  SBC_AY  = 0xF9,
  SBC_IX  = 0xE1,
  SBC_IY  = 0xF1,
  SEC     = 0x38,
  SED     = 0xF8,
  SEI     = 0x78,
  STA_Z   = 0x85,
  STA_ZX  = 0x95,
  STA_A   = 0x8D,
  STA_AX  = 0x9D,
  STA_AY  = 0x99,
  STA_IX  = 0x81,
  STA_IY  = 0x91,
  STX_Z   = 0x86,
  STX_ZY  = 0x96,
  STX_A   = 0x8E,
  STY_Z   = 0x84,
  STY_ZX  = 0x94,
  STY_A   = 0x8C,
  TAX     = 0xAA,
  TAY     = 0xA8,
  TSX     = 0xBA,
  TXA     = 0x8A,
  TXS     = 0x9A,
  TYA     = 0x98,
} Instruction;

typedef struct {
  u16 PC;
  u8 SP;
  u8 A;
  u8 X;
  u8 Y;

  union {
    u8 PS;

    struct {
      u8 CF : 1;
      u8 ZF : 1;
      u8 IF : 1;
      u8 DF : 1;
      u8 BF : 1;
      u8 UF : 1;
      u8 VF : 1;
      u8 NF : 1;
    };
  };

  u8 cycles;
} CPU;

extern CPU cpu;

void cpu_exec();

void cpu_reset();
void cpu_brk();
void cpu_nmi();
void cpu_irq();

#endif // !PROCESSOR_H
