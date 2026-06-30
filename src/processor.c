/**
 *  Processor module for BBC Micro
 *
 *  NMOS 6502 8-bit CPU
 *
 *  Memory notes (from memory.c):
 *    0x0000 - 0x00FF: Zero Page
 *    0x0100 - 0x01FF: Stack
 *    0x0200 - 0x7FFF: Free memory
 *    0xFFFA - 0xFFFF: Reserved for resets and interrupts
 *      A-B: non-maskable interrupt handler
 *      C-D: power on reset location
 *      E-F: BRK / interrupt request handler
 *
 *  Registers:
 *    Program Counter (PC): 16-bit
 *    Stack Pointer (SP): 8-bit, starts at FF (0x01FF) and decremented to 00
 *      (0x0100)
 *      6502 uses the 'push' and 'pull' convention for stack operations
 *    Accumulator (A): 8-bit, used in arithmetic and logical operations
 *    Index Register X (X): 8-bit, used to hold counters or offsets for accesing
 *      memory. Special function: can be used to get copy of SP or change its
 *      value
 *    Index Register Y (Y): 8-bit, same as X but no special function
 *    Processor Status (PS): 7-bit bitfield
 *      Carry Flag (C): set if overflow from bit 7 or underflow from bit 0
 *      Zero Flag (Z): set if result of last operation was 0
 *      Interrupt Disable (I): set by 'Set Interrupt Disable' (SEI), cleared by
 *        'Clear Interrupt Disable' (CLI)
 *      Decimal Mode (D): set by 'Set Decimal Flag' (SED), cleared by
 *        'Clear Decimal FLag' (CLD)
 *      Break Command (B): set by BRK
 *      Overflow Flag (O): set if result yielded invalid 2's complement result
 *      Negative Flag (N): set if bit 7 had been set to 1
 */

#include <stdio.h>

#include "fdc.h"
#include "memory.h"
#include "processor.h"

CPU cpu = {0};

static inline void set_NZ(u8 result) {
  cpu.NF = (result & 0x80) != 0;
  cpu.ZF = result == 0;
}

static inline u8 calc_overflow(u8 a, u8 operand, u16 result) {
  return (~(a ^ operand) & (a ^ (u8)result) & 0x80) != 0;
}

static inline u16 zero() { return mem_read(cpu.PC++); }

static inline u16 zero_x() { return (mem_read(cpu.PC++) + cpu.X) & 0xFF; }

static inline u16 zero_y() { return (mem_read(cpu.PC++) + cpu.Y) & 0xFF; }

static inline u16 abs_() {
  u16 absolute_addr = READ_16LE(cpu.PC);
  cpu.PC += 2;
  return absolute_addr;
}

static inline u16 abs_x() {
  u16 absolute_addr = READ_16LE(cpu.PC);
  cpu.PC += 2;
  cpu.crossed_page = (absolute_addr & 0xFF) + cpu.X > 0xFF;
  return absolute_addr + cpu.X;
}

static inline u16 abs_y() {
  u16 absolute_addr = READ_16LE(cpu.PC);
  cpu.PC += 2;
  cpu.crossed_page = (absolute_addr & 0xFF) + cpu.Y > 0xFF;
  return absolute_addr + cpu.Y;
}

static inline u16 ind() {
  u16 effective_addr;
  u16 indirect_addr = READ_16LE(cpu.PC);
  if ((indirect_addr & 0xFF) == 0xFF) {
    effective_addr =
        mem_read(indirect_addr ^ 0xFF) << 8 | mem_read(indirect_addr);
  } else {
    effective_addr = READ_16LE(indirect_addr);
  }
  cpu.PC += 2;
  return effective_addr;
}

static inline u16 ind_x() {
  u8 zero_page_addr = mem_read(cpu.PC++);
  return READ_16LE((zero_page_addr + cpu.X) & 0xFF);
}

static inline u16 ind_y() {
  u8 zero_page_addr = mem_read(cpu.PC++);
  u16 contents      = READ_16LE(zero_page_addr);
  cpu.crossed_page  = (contents & 0xFF) + cpu.Y > 0xFF;
  return contents + cpu.Y;
}

static inline void adc(u16 addr) {
  u8 operand        = mem_read(addr);
  u16 binary_result = cpu.A + operand + cpu.CF;

  // NOTE: NMOS 6502 incorrectly calculates all flags from the binary result
  // even in decimal mode. This behaviour is replicated in the
  // emulation to mimic the BBCMicro's 6502A cpu

  cpu.VF = calc_overflow(cpu.A, operand, binary_result);
  set_NZ((u8)binary_result);

  if (cpu.DF) {
    u8 low = (cpu.A & 0x0F) + (operand & 0x0F) + cpu.CF;
    // wrap back to 0-9 range in the first nibble
    if (low > 9)
      low += 6;

    u8 high = (cpu.A >> 4) + (operand >> 4) + (low > 0x0F);
    // wrap back to 0-9 range in the first nibble
    if (high > 9)
      high += 6;

    cpu.A  = ((high & 0x0F) << 4) | (low & 0x0F);
    cpu.CF = high > 0x0F;
  } else {
    cpu.A  = (u8)binary_result;
    cpu.CF = binary_result > 0xFF;
  }
}

static inline void and(u16 addr) {
  cpu.A &= mem_read(addr);
  set_NZ(cpu.A);
}

static inline void asl(u16 addr) {
  u8 value  = mem_read(addr);
  u8 result = value << 1;
  mem_write(addr, result);
  cpu.CF = (value & 0x80) != 0;
  set_NZ(result);
}

static inline void branch() {
  cpu.cycles++;
  u8 value = mem_read(cpu.PC++);
  if (((cpu.PC + (i8)value) ^ cpu.PC) & 0xFF00)
    cpu.cycles++;
  cpu.PC += (i8)value;
}

static inline void bit(u16 addr) {
  u8 operand = mem_read(addr);
  cpu.ZF     = (cpu.A & operand) == 0;
  cpu.VF     = (operand & 0x40) != 0;
  cpu.NF     = (operand & 0x80) != 0;
}

static inline void cmp(u16 addr) {
  u8 operand = mem_read(addr);
  u8 result  = cpu.A - operand;
  cpu.CF     = cpu.A >= operand;
  set_NZ(result);
}

static inline void cpx(u16 addr) {
  u8 operand = mem_read(addr);
  u8 result  = cpu.X - operand;
  cpu.CF     = cpu.X >= operand;
  set_NZ(result);
}

static inline void cpy(u16 addr) {
  u8 operand = mem_read(addr);
  u8 result  = cpu.Y - operand;
  cpu.CF     = cpu.Y >= operand;
  set_NZ(result);
}

static inline void dec(u16 addr) {
  u8 result = mem_read(addr) - 1;
  mem_write(addr, result);
  set_NZ(result);
}

static inline void inc(u16 addr) {
  u8 result = mem_read(addr) + 1;
  mem_write(addr, result);
  set_NZ(result);
}

static inline void eor(u16 addr) {
  u8 operand = mem_read(addr);
  cpu.A ^= operand;
  set_NZ(cpu.A);
}

static inline void lda(u16 addr) {
  cpu.A = mem_read(addr);
  set_NZ(cpu.A);
}

static inline void ldx(u16 addr) {
  cpu.X = mem_read(addr);
  set_NZ(cpu.X);
}

static inline void ldy(u16 addr) {
  cpu.Y = mem_read(addr);
  set_NZ(cpu.Y);
}

static inline void lsr(u16 addr) {
  u8 value  = mem_read(addr);
  u8 result = value >> 1;
  mem_write(addr, result);
  cpu.CF = (value & 0x01) != 0;
  set_NZ(result);
}

static inline void ora(u16 addr) {
  u8 operand = mem_read(addr);
  cpu.A |= operand;
  set_NZ(cpu.A);
}

static inline void rol(u16 addr) {
  u8 value  = mem_read(addr);
  u8 result = (value << 1) | cpu.CF;
  mem_write(addr, result);
  cpu.CF = (value & 0x80) != 0;
  set_NZ(result);
}

static inline void ror(u16 addr) {
  u8 value  = mem_read(addr);
  u8 result = (value >> 1) | (cpu.CF << 7);
  mem_write(addr, result);
  cpu.CF = (value & 0x01) != 0;
  set_NZ(result);
}

static inline void sbc(u16 addr) {
  u8 operand        = mem_read(addr);
  u16 binary_result = cpu.A + (operand ^ 0xFF) + cpu.CF;

  cpu.VF = calc_overflow(cpu.A, operand ^ 0xFF, binary_result);
  set_NZ((u8)binary_result);

  if (cpu.DF) {
    u8 low = (cpu.A & 0x0F) - (operand & 0x0F) - !cpu.CF;
    // wrap back to 0-9 range in the first nibble
    u8 borrow = 0;
    if (low > 9) {
      borrow = 1;
      low -= 6;
    }

    u8 high = (cpu.A >> 4) - (operand >> 4) - borrow;
    // wrap back to 0-9 range in the first nibble
    if (high > 9)
      high -= 6;

    cpu.A  = ((high & 0x0F) << 4) | (low & 0x0F);
    cpu.CF = high <= 9;
  } else {
    cpu.A  = (u8)binary_result;
    cpu.CF = binary_result > 0xFF;
  }
}

static inline void sta(u16 addr) { mem_write(addr, cpu.A); }

static inline void stx(u16 addr) { mem_write(addr, cpu.X); }

static inline void sty(u16 addr) { mem_write(addr, cpu.Y); }

void cpu_exec() {
  Instruction opcode = mem_read(cpu.PC++);

  switch (opcode) {
  case ADC_I: {
    cpu.cycles += 2;
    // printf("ADC_I\n");
    adc(cpu.PC++);
  } break;
  case ADC_Z: {
    cpu.cycles += 3;
    // printf("ADC_Z\n");
    adc(zero());
  } break;
  case ADC_ZX: {
    cpu.cycles += 4;
    // printf("ADC_ZX\n");
    adc(zero_x());
  } break;
  case ADC_A: {
    cpu.cycles += 4;
    // printf("ADC_A\n");
    adc(abs_());
  } break;
  case ADC_AX: {
    cpu.cycles += 4;
    // printf("ADC_AX\n");
    adc(abs_x());
    if (cpu.crossed_page)
      cpu.cycles++;
  } break;
  case ADC_AY: {
    cpu.cycles += 4;
    // printf("ADC_AY\n");
    adc(abs_y());
    if (cpu.crossed_page)
      cpu.cycles++;
  } break;
  case ADC_IX: {
    cpu.cycles += 6;
    // printf("ADC_IX\n");
    adc(ind_x());
  } break;
  case ADC_IY: {
    cpu.cycles += 5;
    // printf("ADC_IY\n");
    adc(ind_y());
    if (cpu.crossed_page)
      cpu.cycles++;
  } break;
  case AND_I: {
    cpu.cycles += 2;
    // printf("AND_I\n");
    and(cpu.PC++);
  } break;
  case AND_Z: {
    cpu.cycles += 3;
    // printf("AND_Z\n");
    and(zero());
  } break;
  case AND_ZX: {
    cpu.cycles += 4;
    // printf("AND_ZX\n");
    and(zero_x());
  } break;
  case AND_A: {
    cpu.cycles += 4;
    // printf("AND_A\n");
    and(abs_());
  } break;
  case AND_AX: {
    cpu.cycles += 4;
    // printf("AND_AX\n");
    and(abs_x());
    if (cpu.crossed_page)
      cpu.cycles++;
  } break;
  case AND_AY: {
    cpu.cycles += 4;
    // printf("AND_AY\n");
    and(abs_y());
    if (cpu.crossed_page)
      cpu.cycles++;
  } break;
  case AND_IX: {
    cpu.cycles += 6;
    // printf("AND_IX\n");
    and(ind_x());
  } break;
  case AND_IY: {
    cpu.cycles += 5;
    // printf("AND_IY\n");
    and(ind_y());
    if (cpu.crossed_page)
      cpu.cycles++;
  } break;
  case ASL_ACC: {
    cpu.cycles += 2;
    // printf("ASL_ACC\n");
    cpu.CF = (cpu.A & 0x80) != 0;
    cpu.A <<= 1;
    set_NZ(cpu.A);
  } break;
  case ASL_Z: {
    cpu.cycles += 5;
    // printf("ASL_Z\n");
    asl(zero());
  } break;
  case ASL_ZX: {
    cpu.cycles += 6;
    // printf("ASL_ZX\n");
    asl(zero_x());
  } break;
  case ASL_A: {
    cpu.cycles += 6;
    // printf("ASL_A\n");
    asl(abs_());
  } break;
  case ASL_AX: {
    cpu.cycles += 7;
    // printf("ASL_AX\n");
    asl(abs_x());
  } break;
  case BCC: {
    cpu.cycles += 2;
    // printf("BCC\n");
    if (cpu.CF != 0) {
      cpu.PC++;
      break;
    }

    branch();
  } break;
  case BCS: {
    cpu.cycles += 2;
    /* printf("BCS\n"); */
    if (cpu.CF != 1) {
      cpu.PC++;
      break;
    }

    branch();
  } break;
  case BEQ: {
    cpu.cycles += 2;
    // printf("BEQ\n");
    if (cpu.ZF != 1) {
      cpu.PC++;
      break;
    }

    branch();
  } break;
  case BIT_Z: {
    cpu.cycles += 3;
    // printf("BIT_Z\n");
    bit(zero());
  } break;
  case BIT_A: {
    cpu.cycles += 4;
    // printf("BIT_A\n");
    bit(abs_());
  } break;
  case BMI: {
    cpu.cycles += 2;
    // printf("BMI\n");
    if (cpu.NF != 1) {
      cpu.PC++;
      break;
    }

    branch();
  } break;
  case BNE: {
    cpu.cycles += 2;
    // printf("BNE\n");
    if (cpu.ZF != 0) {
      cpu.PC++;
      break;
    }

    branch();
  } break;
  case BPL: {
    cpu.cycles += 2;
    // printf("BPL ");
    if (cpu.NF != 0) {
      cpu.PC++;
      break;
    }

    branch();
  } break;
  case BRK: {
    cpu.cycles += 7;
    // printf("BRK\n");
    cpu_brk();
  } break;
  case BVC: {
    cpu.cycles += 2;
    // printf("BVC\n");
    if (cpu.VF != 0) {
      cpu.PC++;
      break;
    }

    branch();
  } break;
  case BVS: {
    cpu.cycles += 2;
    // printf("BVS\n");
    if (cpu.VF != 1) {
      cpu.PC++;
      break;
    }

    branch();
  } break;
  case CLC: {
    cpu.cycles += 2;
    // printf("CLC\n");
    cpu.CF = 0;
  } break;
  case CLD: {
    cpu.cycles += 2;
    // printf("CLD\n");
    cpu.DF = 0;
  } break;
  case CLI: {
    cpu.cycles += 2;
    // printf("CLI\n");
    cpu.IF = 0;
  } break;
  case CLV: {
    cpu.cycles += 2;
    // printf("CLV\n");
    cpu.VF = 0;
  } break;
  case CMP_I: {
    cpu.cycles += 2;
    // printf("CMP_I\n");
    cmp(cpu.PC++);
  } break;
  case CMP_Z: {
    cpu.cycles += 3;
    // printf("CMP_Z\n");
    cmp(zero());
  } break;
  case CMP_ZX: {
    cpu.cycles += 4;
    // printf("CMP_ZX\n");
    cmp(zero_x());
  } break;
  case CMP_A: {
    cpu.cycles += 4;
    // printf("CMP_A\n");
    cmp(abs_());
  } break;
  case CMP_AX: {
    cpu.cycles += 4;
    // printf("CMP_AX\n");
    cmp(abs_x());
    if (cpu.crossed_page)
      cpu.cycles++;
  } break;
  case CMP_AY: {
    cpu.cycles += 4;
    // printf("CMP_AY\n");
    cmp(abs_y());
    if (cpu.crossed_page)
      cpu.cycles++;
  } break;
  case CMP_IX: {
    cpu.cycles += 6;
    // printf("CMP_IX\n");
    cmp(ind_x());
  } break;
  case CMP_IY: {
    cpu.cycles += 5;
    // printf("CMP_IY\n");
    cmp(ind_y());
    if (cpu.crossed_page)
      cpu.cycles++;
  } break;
  case CPX_I: {
    cpu.cycles += 2;
    // printf("CPX_I\n");
    cpx(cpu.PC++);
  } break;
  case CPX_Z: {
    cpu.cycles += 3;
    // printf("CPX_Z\n");
    cpx(zero());
  } break;
  case CPX_A: {
    cpu.cycles += 4;
    // printf("CPX_A\n");
    cpx(abs_());
  } break;
  case CPY_I: {
    cpu.cycles += 2;
    // printf("CPY_I\n");
    cpy(cpu.PC++);
  } break;
  case CPY_Z: {
    cpu.cycles += 3;
    // printf("CPY_Z\n");
    cpy(zero());
  } break;
  case CPY_A: {
    cpu.cycles += 4;
    // printf("CPY_A\n");
    cpy(abs_());
  } break;
  case DEC_Z: {
    cpu.cycles += 5;
    // printf("DEC_Z\n");
    dec(zero());
  } break;
  case DEC_ZX: {
    cpu.cycles += 6;
    // printf("DEC_ZX\n");
    dec(zero_x());
  } break;
  case DEC_A: {
    cpu.cycles += 6;
    // printf("DEC_A\n");
    dec(abs_());
  } break;
  case DEC_AX: {
    cpu.cycles += 7;
    /* printf("DEC_AX "); */
    dec(abs_x());
  } break;
  case DEX: {
    cpu.cycles += 2;
    /* printf("DEX %d\n", cpu.X); */
    cpu.X -= 1;
    set_NZ(cpu.X);
  } break;
  case DEY: {
    cpu.cycles += 2;
    // printf("DEY\n");
    cpu.Y -= 1;
    set_NZ(cpu.Y);
  } break;
  case EOR_I: {
    cpu.cycles += 2;
    // printf("EOR_I\n");
    eor(cpu.PC++);
  } break;
  case EOR_Z: {
    cpu.cycles += 3;
    // printf("EOR_Z\n");
    eor(zero());
  } break;
  case EOR_ZX: {
    cpu.cycles += 4;
    // printf("EOR_ZX\n");
    eor(zero_x());
  } break;
  case EOR_A: {
    cpu.cycles += 4;
    // printf("EOR_A\n");
    eor(abs_());
  } break;
  case EOR_AX: {
    cpu.cycles += 4;
    // printf("EOR_AX\n");
    eor(abs_x());
    if (cpu.crossed_page)
      cpu.cycles++;
  } break;
  case EOR_AY: {
    cpu.cycles += 4;
    // printf("EOR_AY\n");
    eor(abs_y());
    if (cpu.crossed_page)
      cpu.cycles++;
  } break;
  case EOR_IX: {
    cpu.cycles += 6;
    // printf("EOR_IX\n");
    eor(ind_x());
  } break;
  case EOR_IY: {
    cpu.cycles += 5;
    // printf("EOR_IY\n");
    eor(ind_y());
    if (cpu.crossed_page)
      cpu.cycles++;
  } break;
  case INC_Z: {
    cpu.cycles += 5;
    // printf("INC_Z\n");
    inc(zero());
  } break;
  case INC_ZX: {
    cpu.cycles += 6;
    // printf("INC_ZX\n");
    inc(zero_x());
  } break;
  case INC_A: {
    cpu.cycles += 6;
    // printf("INC_A\n");
    inc(abs_());
  } break;
  case INC_AX: {
    cpu.cycles += 7;
    /* printf("INC_AX "); */
    inc(abs_x());
  } break;
  case INX: {
    cpu.cycles += 2;
    // printf("INX\n");
    cpu.X += 1;
    set_NZ(cpu.X);
  } break;
  case INY: {
    cpu.cycles += 2;
    // printf("INY\n");
    cpu.Y += 1;
    set_NZ(cpu.Y);
  } break;
  case JMP_A: {
    cpu.cycles += 3;
    // printf("JMP_A\n");
    cpu.PC = abs_();
  } break;
  case JMP_IND: {
    cpu.cycles += 5;
    // printf("JMP_IND\n");
    cpu.PC = ind();
  } break;
  case JSR: {
    cpu.cycles += 6;
    // printf("JSR\n");
    stack_push_16(cpu.PC + 1);
    cpu.PC = abs_();
  } break;
  case LDA_I: {
    cpu.cycles += 2;
    // printf("LDA_I\n");
    lda(cpu.PC++);
  } break;
  case LDA_Z: {
    cpu.cycles += 3;
    // printf("LDA_Z\n");
    lda(zero());
  } break;
  case LDA_ZX: {
    cpu.cycles += 4;
    // printf("LDA_ZX\n");
    lda(zero_x());
  } break;
  case LDA_A: {
    cpu.cycles += 4;
    // printf("LDA_A\n");
    lda(abs_());
  } break;
  case LDA_AX: {
    cpu.cycles += 4;
    // printf("LDA_AX\n");
    lda(abs_x());
    if (cpu.crossed_page)
      cpu.cycles++;
  } break;
  case LDA_AY: {
    cpu.cycles += 4;
    // printf("LDA_AY\n");
    lda(abs_y());
    if (cpu.crossed_page)
      cpu.cycles++;
  } break;
  case LDA_IX: {
    cpu.cycles += 6;
    // printf("LDA_IX\n");
    lda(ind_x());
  } break;
  case LDA_IY: {
    cpu.cycles += 5;
    // printf("LDA_IY\n");
    lda(ind_y());
    if (cpu.crossed_page)
      cpu.cycles++;
  } break;
  case LDX_I: {
    cpu.cycles += 2;
    // printf("LDX_I\n");
    ldx(cpu.PC++);
  } break;
  case LDX_Z: {
    cpu.cycles += 3;
    // printf("LDX_Z\n");
    ldx(zero());
  } break;
  case LDX_ZY: {
    cpu.cycles += 4;
    // printf("LDX_ZY\n");
    ldx(zero_y());
  } break;
  case LDX_A: {
    cpu.cycles += 4;
    // printf("LDX_A\n");
    ldx(abs_());
  } break;
  case LDX_AY: {
    cpu.cycles += 4;
    // printf("LDX_AY\n");
    ldx(abs_y());
    if (cpu.crossed_page)
      cpu.cycles++;
  } break;
  case LDY_I: {
    cpu.cycles += 2;
    // printf("LDY_I\n");
    ldy(cpu.PC++);
  } break;
  case LDY_Z: {
    cpu.cycles += 3;
    // printf("LDY_Z\n");
    ldy(zero());
  } break;
  case LDY_ZX: {
    cpu.cycles += 4;
    // printf("LDY_ZX\n");
    ldy(zero_x());
  } break;
  case LDY_A: {
    cpu.cycles += 4;
    // printf("LDY_A\n");
    ldy(abs_());
  } break;
  case LDY_AX: {
    cpu.cycles += 4;
    // printf("LDY_AX\n");
    ldy(abs_x());
    if (cpu.crossed_page)
      cpu.cycles++;
  } break;
  case LSR_ACC: {
    cpu.cycles += 2;
    // printf("LSR_ACC\n");
    cpu.CF = (cpu.A & 0x01) != 0;
    cpu.A >>= 1;
    set_NZ(cpu.A);
  } break;
  case LSR_Z: {
    cpu.cycles += 5;
    // printf("LSR_Z\n");
    lsr(zero());
  } break;
  case LSR_ZX: {
    cpu.cycles += 6;
    // printf("LSR_ZX\n");
    lsr(zero_x());
  } break;
  case LSR_A: {
    cpu.cycles += 6;
    // printf("LSR_A\n");
    lsr(abs_());
  } break;
  case LSR_AX: {
    cpu.cycles += 7;
    // printf("LSR_AX\n");
    lsr(abs_x());
  } break;
  case NOP: {
    cpu.cycles += 2;
    // printf("NOP\n");
  } break;
  case ORA_I: {
    cpu.cycles += 2;
    // printf("ORA_I\n");
    ora(cpu.PC++);
  } break;
  case ORA_Z: {
    cpu.cycles += 3;
    // printf("ORA_Z\n");
    ora(zero());
  } break;
  case ORA_ZX: {
    cpu.cycles += 4;
    // printf("ORA_ZX\n");
    ora(zero_x());
  } break;
  case ORA_A: {
    cpu.cycles += 4;
    // printf("ORA_A\n");
    ora(abs_());
  } break;
  case ORA_AX: {
    cpu.cycles += 4;
    // printf("ORA_AX\n");
    ora(abs_x());
    if (cpu.crossed_page)
      cpu.cycles++;
  } break;
  case ORA_AY: {
    cpu.cycles += 4;
    // printf("ORA_AY\n");
    ora(abs_y());
    if (cpu.crossed_page)
      cpu.cycles++;
  } break;
  case ORA_IX: {
    cpu.cycles += 6;
    // printf("ORA_IX\n");
    ora(ind_x());
  } break;
  case ORA_IY: {
    cpu.cycles += 5;
    // printf("ORA_IY\n");
    ora(ind_y());
    if (cpu.crossed_page)
      cpu.cycles++;
  } break;
  case PHA: {
    cpu.cycles += 3;
    // printf("PHA\n");
    stack_push(cpu.A);
  } break;
  case PHP: {
    cpu.cycles += 3;
    // printf("PHP\n");
    stack_push(cpu.PS);
  } break;
  case PLA: {
    cpu.cycles += 4;
    // printf("PLA\n");
    cpu.A = stack_pull();
    set_NZ(cpu.A);
  } break;
  case PLP: {
    cpu.cycles += 4;
    // printf("PLP\n");
    cpu.PS = stack_pull() & ~0x10;
  } break;
  case ROL_ACC: {
    cpu.cycles += 2;
    // printf("ROL_ACC\n");
    u8 old_cf = cpu.CF;
    cpu.CF    = (cpu.A & 0x80) != 0;
    cpu.A     = (cpu.A << 1) | old_cf;
    set_NZ(cpu.A);
  } break;
  case ROL_Z: {
    cpu.cycles += 5;
    // printf("ROL_Z\n");
    rol(zero());
  } break;
  case ROL_ZX: {
    cpu.cycles += 6;
    // printf("ROL_ZX\n");
    rol(zero_x());
  } break;
  case ROL_A: {
    cpu.cycles += 6;
    // printf("ROL_A\n");
    rol(abs_());
  } break;
  case ROL_AX: {
    cpu.cycles += 7;
    // printf("ROL_AX\n");
    rol(abs_x());
  } break;
  case ROR_ACC: {
    cpu.cycles += 2;
    // printf("ROR_ACC\n");
    u8 old_cf = cpu.CF;
    cpu.CF    = (cpu.A & 0x01) != 0;
    cpu.A     = (cpu.A >> 1) | (old_cf << 7);
    set_NZ(cpu.A);
  } break;
  case ROR_Z: {
    cpu.cycles += 5;
    // printf("ROR_Z\n");
    ror(zero());
  } break;
  case ROR_ZX: {
    cpu.cycles += 6;
    // printf("ROR_ZX\n");
    ror(zero_x());
  } break;
  case ROR_A: {
    cpu.cycles += 6;
    // printf("ROR_A\n");
    ror(abs_());
  } break;
  case ROR_AX: {
    cpu.cycles += 7;
    // printf("ROR_AX\n");
    ror(abs_x());
  } break;
  case RTI: {
    cpu.cycles += 6;
    // printf("RTI\n");
    cpu.PS = stack_pull() & ~0x10;
    cpu.PC = stack_pull_16();
  } break;
  case RTS: {
    cpu.cycles += 6;
    // printf("RTS\n");
    cpu.PC = stack_pull_16() + 1;
  } break;
  case SBC_I: {
    cpu.cycles += 2;
    /* printf("SBC_I\n"); */
    sbc(cpu.PC++);
  } break;
  case SBC_Z: {
    cpu.cycles += 3;
    /* printf("SBC_Z\n"); */
    sbc(zero());
  } break;
  case SBC_ZX: {
    cpu.cycles += 4;
    /* printf("SBC_ZX\n"); */
    sbc(zero_x());
  } break;
  case SBC_A: {
    cpu.cycles += 4;
    /* printf("SBC_A\n"); */
    sbc(abs_());
  } break;
  case SBC_AX: {
    cpu.cycles += 4;
    /* printf("SBC_AX\n"); */
    sbc(abs_x());
    if (cpu.crossed_page)
      cpu.cycles++;
  } break;
  case SBC_AY: {
    cpu.cycles += 4;
    /* printf("SBC_AY\n"); */
    sbc(abs_y());
    if (cpu.crossed_page)
      cpu.cycles++;
  } break;
  case SBC_IX: {
    cpu.cycles += 6;
    /* printf("SBC_IX\n"); */
    sbc(ind_x());
  } break;
  case SBC_IY: {
    cpu.cycles += 5;
    /* printf("SBC_IY\n"); */
    sbc(ind_y());
    if (cpu.crossed_page)
      cpu.cycles++;
  } break;
  case SEC: {
    cpu.cycles += 2;
    // printf("SEC\n");
    cpu.CF = 1;
  } break;
  case SED: {
    cpu.cycles += 2;
    // printf("SED\n");
    cpu.DF = 1;
  } break;
  case SEI: {
    cpu.cycles += 2;
    // printf("SEI\n");
    cpu.IF = 1;
  } break;
  case STA_Z: {
    cpu.cycles += 3;
    // printf("STA_Z\n");
    sta(zero());
  } break;
  case STA_ZX: {
    cpu.cycles += 4;
    // printf("STA_ZX\n");
    sta(zero_x());
  } break;
  case STA_A: {
    cpu.cycles += 4;
    // printf("STA_A\n");
    sta(abs_());
  } break;
  case STA_AX: {
    cpu.cycles += 5;
    // printf("STA_AX\n");
    sta(abs_x());
  } break;
  case STA_AY: {
    cpu.cycles += 5;
    // printf("STA_AY\n");
    sta(abs_y());
  } break;
  case STA_IX: {
    cpu.cycles += 6;
    // printf("STA_IX\n");
    sta(ind_x());
  } break;
  case STA_IY: {
    cpu.cycles += 6;
    // printf("STA_IY\n");
    sta(ind_y());
  } break;
  case STX_Z: {
    cpu.cycles += 3;
    // printf("STX_Z\n");
    stx(zero());
  } break;
  case STX_ZY: {
    cpu.cycles += 4;
    // printf("STX_ZY\n");
    stx(zero_y());
  } break;
  case STX_A: {
    cpu.cycles += 4;
    // printf("STX_A\n");
    stx(abs_());
  } break;
  case STY_Z: {
    cpu.cycles += 3;
    // printf("STY_Z\n");
    sty(zero());
  } break;
  case STY_ZX: {
    cpu.cycles += 4;
    // printf("STY_ZX\n");
    sty(zero_x());
  } break;
  case STY_A: {
    cpu.cycles += 4;
    // printf("STY_A\n");
    sty(abs_());
  } break;
  case TAX: {
    cpu.cycles += 2;
    // printf("TAX\n");
    cpu.X = cpu.A;
    set_NZ(cpu.X);
  } break;
  case TAY: {
    cpu.cycles += 2;
    // printf("TAY\n");
    cpu.Y = cpu.A;
    set_NZ(cpu.Y);
  } break;
  case TSX: {
    cpu.cycles += 2;
    // printf("TSX\n");
    cpu.X = cpu.SP;
    set_NZ(cpu.X);
  } break;
  case TXA: {
    cpu.cycles += 2;
    // printf("TXA\n");
    cpu.A = cpu.X;
    set_NZ(cpu.A);
  } break;
  case TXS: {
    cpu.cycles += 2;
    // printf("TXS\n");
    cpu.SP = cpu.X;
  } break;
  case TYA: {
    cpu.cycles += 2;
    // printf("TYA\n");
    cpu.A = cpu.Y;
    set_NZ(cpu.A);
  } break;
  }

  // service FDC inter-byte timing, then check for interrupts

  fdc_tick();

  if (cpu.nmi_pending)
    cpu_nmi();
}

void cpu_reset() {
  // system initialisation time of 6 clock cycles

  cpu.cycles       = 6;
  cpu.crossed_page = 0;

  // mask interrupt flag set

  cpu.IF = 1;

  // load PC from FFFC and FFFD

  cpu.PC = READ_16LE(0XFFFC);

  cpu.SP = 0xFF;
  cpu.A  = 0;
  cpu.X  = 0;
  cpu.Y  = 0;
  cpu.PS = 0;
  cpu.UF = 1; // set unused bit to 1

  cpu.nmi_pending = 0;
}

void cpu_brk() {
  if (cpu.IF)
    return;

  cpu.BF = 1;
  cpu.UF = 1;

  stack_push_16(cpu.PC + 1);
  stack_push(cpu.PS);

  cpu.DF = 0;
  cpu.IF = 1;
  cpu.PC = READ_16LE(0xFFFE);
  cpu.BF = 1;
}

void cpu_nmi() {
  cpu.UF = 1;

  stack_push_16(cpu.PC);
  stack_push(cpu.PS);

  cpu.DF = 0;
  cpu.IF = 1;
  cpu.PC = READ_16LE(0xFFFA);

  cpu.nmi_pending = 0;
}

void cpu_irq() {
  if (cpu.IF)
    return;

  cpu.UF = 1;

  stack_push_16(cpu.PC);
  stack_push(cpu.PS);

  cpu.DF = 0;
  cpu.IF = 1;
  cpu.PC = READ_16LE(0xFFFE);
}
