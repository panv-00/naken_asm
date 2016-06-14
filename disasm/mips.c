/**
 *  naken_asm assembler.
 *  Author: Michael Kohn
 *   Email: mike@mikekohn.net
 *     Web: http://www.mikekohn.net/
 * License: GPL
 *
 * Copyright 2010-2016 by Michael Kohn
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "disasm/mips.h"

#define READ_RAM(a) memory_read_m(memory, a)

int get_cycle_count_mips(uint32_t opcode)
{
  return -1;
}

static int disasm_vector(struct _memory *memory, uint32_t address, char *instruction, int *cycles_min, int *cycles_max)
{
  uint32_t opcode;
  int n, r;
  char temp[32];
  int ft, fs, fd, dest;
  //int16_t offset;
  int immediate;

  opcode = memory_read32_m(memory, address);

  instruction[0] = 0;

  n = 0;
  while(mips_ee_vector[n].instr != NULL)
  {
    if (mips_ee_vector[n].opcode == (opcode & mips_ee_vector[n].mask))
    {
      strcpy(instruction, mips_ee_vector[n].instr);

      dest = (opcode >> 21) & 0xf;
      ft = (opcode >> 16) & 0x1f;
      fs = (opcode >> 11) & 0x1f;
      fd = (opcode >> 6) & 0x1f;

      if ((mips_ee_vector[n].flags & FLAG_DEST) != 0)
      {
        strcat(instruction, ".");
        if ((dest & 8) != 0) { strcat(instruction, "x"); }
        if ((dest & 4) != 0) { strcat(instruction, "y"); }
        if ((dest & 2) != 0) { strcat(instruction, "z"); }
        if ((dest & 1) != 0) { strcat(instruction, "w"); }
      }

      for (r = 0; r < mips_ee_vector[n].operand_count; r++)
      {
        if (r != 0) { strcat(instruction, ","); }

        switch(mips_ee_vector[n].operand[r])
        {
          case MIPS_OP_VFT:
            sprintf(temp, " $vf%d", ft);
            break;
          case MIPS_OP_VFS:
            sprintf(temp, " $vf%d", fs);
            break;
          case MIPS_OP_VFD:
            sprintf(temp, " $vf%d", fd);
            break;
          case MIPS_OP_VIT:
            sprintf(temp, " $vi%d", ft);
            break;
          case MIPS_OP_VIS:
            sprintf(temp, " $vi%d", fs);
            break;
          case MIPS_OP_VID:
            sprintf(temp, " $vi%d", fd);
            break;
          case MIPS_OP_VI01:
            sprintf(temp, " $vi01");
            break;
          case MIPS_OP_VI27:
            sprintf(temp, " $vi27");
            break;
          case MIPS_OP_I:
            strcpy(temp, " I");
            break;
          case MIPS_OP_Q:
            strcpy(temp, " Q");
            break;
          case MIPS_OP_P:
            strcpy(temp, " P");
            break;
          case MIPS_OP_R:
            strcpy(temp, " R");
            break;
          case MIPS_OP_ACC:
            strcpy(temp, " ACC");
            break;
#if 0
          case MIPS_OP_OFFSET:
            offset = (opcode & 0x7ff) << 3;
            if ((offset & 0x400) != 0) { offset |= 0xf800; }
            sprintf(temp, " 0x%x (offset=%d)", address + 8 + offset, offset);
            break;
          case MIPS_OP_OFFSET_BASE:
            offset = opcode & 0x7ff;
            if ((offset & 0x400) != 0) { offset |= 0xf800; }
            sprintf(temp, " %d(vi%d)", offset, (opcode >> 11) & 0x1f);
            break;
          case MIPS_OP_BASE:
            sprintf(temp, " (vi%d)", fs);
            break;
          case MIPS_OP_BASE_DEC:
            sprintf(temp, " (--vi%d)", fs);
            break;
          case MIPS_OP_BASE_INC:
            sprintf(temp, " (vi%d++)", fs);
            break;
          case MIPS_OP_IMMEDIATE24:
            sprintf(temp, " 0x%06x", opcode & 0xffffff);
            break;
          case MIPS_OP_IMMEDIATE15:
            immediate = (opcode & (0xf << 21)) >> 10;
            immediate |= opcode & 0x7ff;
            sprintf(temp, " 0x%04x", immediate);
            break;
#endif
          case MIPS_OP_IMMEDIATE15_2:
            immediate = (opcode >> 6) & 0x7ff;
            sprintf(temp, " 0x%04x", immediate << 3);
            break;
#if 0
          case MIPS_OP_IMMEDIATE12:
            immediate = (opcode & (1 << 21)) >> 10;
            immediate |= opcode & 0x7ff;
            sprintf(temp, " 0x%03x", immediate);
            break;
#endif
          case MIPS_OP_IMMEDIATE5:
            immediate = (opcode >> 6) & 0x1f;
            if ((immediate & 0x10) != 0) { immediate |= 0xfffffff0; }
            sprintf(temp, " %d", immediate);
            break;
          default:
            strcpy(temp, " ?");
            break;
        }

        strcat(instruction, temp);
      }

      return 4;
    }

    n++;
  }

  strcpy(instruction, "???");

  return 4;
}

int disasm_mips(struct _memory *memory, uint32_t flags, uint32_t address, char *instruction, int *cycles_min, int *cycles_max)
{
  uint32_t opcode;
  int function, format, operation;
  int n, r;
  char temp[32];
  const char *reg[32] =
  {
    "$0", "$at", "$v0", "$v1", "$a0", "$a1", "$a2", "$a3",
    "$t0", "$t1", "$t2", "$t3", "$t4", "$t5", "$t6", "$t7",
    "$s0", "$s1", "$s2", "$s3", "$s4", "$s5", "$s6", "$s7",
    "$t8", "$t9", "$k0", "$k1", "$gp", "$sp", "$fp", "$ra"
  };
  int rs, rt, rd, sa, wt, ws, wd;
  int immediate;

  *cycles_min = 1;
  *cycles_max = 1;
  opcode = memory_read32_m(memory, address);

  instruction[0] = 0;

  if (opcode == 0)
  {
    strcpy(instruction, "nop");
    return 4;
  }

  format = (opcode >> 26) & 0x3f;

  if (format == FORMAT_SPECIAL0 ||
      format == FORMAT_SPECIAL2 ||
      format == FORMAT_SPECIAL3)
  {
    // Special2 / Special3
    function = opcode & 0x3f;

    n = 0;
    while(mips_special_table[n].instr != NULL)
    {
      // Check of this specific MIPS chip uses this instruction.
      if ((mips_special_table[n].version & flags) == 0)
      {
        n++;
        continue;
      }

      if (mips_special_table[n].format == format &&
          mips_special_table[n].function == function)
      {
        uint8_t operand_reg[4] = { 0 };
        int shift;

        if (mips_special_table[n].type == SPECIAL_TYPE_REGS)
        {
          operation = (opcode >> 6) & 0x1f;
          shift = 21;
        }
          else
        if (mips_special_table[n].type == SPECIAL_TYPE_SA)
        {
          operation = (opcode >> 21) & 0x1f;
          shift = 16;
        }
          else
        if (mips_special_table[n].type == SPECIAL_TYPE_BITS ||
            mips_special_table[n].type == SPECIAL_TYPE_BITS2)
        {
          operation = 0;
          shift = 21;
        }
          else
        {
          sprintf(instruction, "internal error");
          return 4;
        }

        if (mips_special_table[n].operation != operation)
        {
          n++;
          continue;
        }

        for (r = 0; r < 4; r++)
        {
          int operand_index = mips_special_table[n].operand[r];

          if (operand_index != -1)
          {
            operand_reg[operand_index] = (opcode >> shift) & 0x1f;
          }

          if (r == 2 && mips_special_table[n].type == SPECIAL_TYPE_BITS)
          {
            operand_reg[operand_index]++;
          }
            else
          if (r == 3 && mips_special_table[n].type == SPECIAL_TYPE_BITS2)
          {
            operand_reg[operand_index + 1] -= operand_reg[operand_index];
            operand_reg[operand_index + 1]++;
          }

          shift -= 5;
        }

        strcpy(instruction, mips_special_table[n].instr);

        for (r = 0; r < mips_special_table[n].operand_count; r++)
        {
          if (r < 2 || mips_special_table[n].type == SPECIAL_TYPE_REGS)
          {
            sprintf(temp, "%s", reg[(int)operand_reg[r]]);
          }
            else
          {
            sprintf(temp, "%d", operand_reg[r]);
          }

          if (r != 0) { strcat(instruction, ", "); }
          else { strcat(instruction, " "); }

          strcat(instruction, temp);
        }

        return 4;
      }

      n++;
    }
  }

  n = 0;
  while(mips_other[n].instr != NULL)
  {
    // Check of this specific MIPS chip uses this instruction.
    if ((mips_other[n].version & flags) == 0)
    {
      n++;
      continue;
    }

    if (mips_other[n].opcode == (opcode & mips_other[n].mask))
    {
      strcpy(instruction, mips_other[n].instr);

      rs = (opcode >> 21) & 0x1f;
      rt = (opcode >> 16) & 0x1f;
      rd = (opcode >> 11) & 0x1f;
      sa = (opcode >> 6) & 0x1f;
      immediate = opcode & 0xffff;

      for (r = 0; r < mips_other[n].operand_count; r++)
      {
        if (r != 0) { strcat(instruction, ","); }

        switch(mips_other[n].operand[r])
        {
          case MIPS_OP_RS:
            sprintf(temp, " %s", reg[rs]);
            break;
          case MIPS_OP_RT:
            sprintf(temp, " %s", reg[rt]);
            break;
          case MIPS_OP_RD:
            sprintf(temp, " %s", reg[rd]);
            break;
          case MIPS_OP_FT:
            sprintf(temp, " $f%d", rt);
            break;
          case MIPS_OP_FS:
            sprintf(temp, " $f%d", rd);
            break;
          case MIPS_OP_FD:
            sprintf(temp, " $f%d", sa);
            break;
          case MIPS_OP_VIS:
            sprintf(temp, " $vi%d", rs);
            break;
          case MIPS_OP_VFT:
            sprintf(temp, " $vf%d", rt);
            break;
          case MIPS_OP_SA:
            sprintf(temp, " %d", sa);
            break;
          case MIPS_OP_IMMEDIATE_SIGNED:
            sprintf(temp, " %d", (int16_t)immediate);
            break;
          case MIPS_OP_IMMEDIATE_RS:
            sprintf(temp, " %d(%s)", (int16_t)immediate, reg[rs]);
            break;
          case MIPS_OP_LABEL:
            if ((immediate & 0x8000) != 0) { immediate |= 0xffff0000; }
            immediate = immediate << 2;
            sprintf(temp, " 0x%08x (%d)", address + 4 + immediate, immediate);
            break;
          case MIPS_OP_PREG:
            sprintf(temp, " %d", (immediate >> 1) & 0x1f);
            break;
          default:
            strcpy(temp, " ?");
            break;
        }

        strcat(instruction, temp);
      }

      return 4;
    }

    n++;
  }

  n = 0;
  while(mips_msa[n].instr != NULL)
  {
    // Check of this specific MIPS chip uses this instruction.
    if ((mips_msa[n].version & flags) == 0)
    {
      n++;
      continue;
    }

    if (mips_msa[n].opcode == (opcode & mips_msa[n].mask))
    {
      strcpy(instruction, mips_msa[n].instr);

      wt = (opcode >> 16) & 0x1f;
      ws = (opcode >> 11) & 0x1f;
      wd = (opcode >> 6) & 0x1f;

      for (r = 0; r < mips_msa[n].operand_count; r++)
      {
        if (r != 0) { strcat(instruction, ","); }

        switch(mips_msa[n].operand[r])
        {
          case MIPS_OP_WT:
            sprintf(temp, " $w%d", wt);
            break;
          case MIPS_OP_WS:
            sprintf(temp, " $w%d", ws);
            break;
          case MIPS_OP_WD:
            sprintf(temp, " $w%d", wd);
            break;
          default:
            strcpy(temp, " ?");
            break;
        }

        strcat(instruction, temp);
      }

      return 4;
    }

    n++;
  }

  n = 0;
  while(mips_branch_table[n].instr != NULL)
  {
    // Check of this specific MIPS chip uses this instruction.
    if ((mips_branch_table[n].version & flags) == 0)
    {
      n++;
      continue;
    }

    if (mips_branch_table[n].op_rt == -1)
    {
      if ((opcode >> 26) == mips_branch_table[n].opcode)
      {
        rs = (opcode >> 21) & 0x1f;
        rt = (opcode >> 16) & 0x1f;
        int16_t offset = (opcode & 0xffff) << 2;

        sprintf(instruction, "%s %s, %s, 0x%x (offset=%d)", mips_branch_table[n].instr, reg[rs], reg[rt],  address + 4 + offset, offset);

        return 4;
      }
    }
      else
    {
      if ((opcode >> 26) == mips_branch_table[n].opcode &&
         ((opcode >> 16) & 0x1f) == mips_branch_table[n].op_rt)
      {
        rs = (opcode >> 21) & 0x1f;
        int16_t offset = (opcode & 0xffff) << 2;

        sprintf(instruction, "%s %s, 0x%x (offset=%d)", mips_branch_table[n].instr, reg[rs], address + 4 + offset, offset);

        return 4;
      }
    }
    n++;
  }

  if (format == 0)
  {
    // R-Type Instruction [ op 6, rs 5, rt 5, rd 5, sa 5, function 6 ]
    function = opcode & 0x3f;
    n = 0;
    while(mips_r_table[n].instr != NULL)
    {
      // Check of this specific MIPS chip uses this instruction.
      if ((mips_r_table[n].version & flags) == 0)
      {
        n++;
        continue;
      }

      if (mips_r_table[n].function == function)
      {
        rs = (opcode >> 21) & 0x1f;
        rt = (opcode >> 16) & 0x1f;
        rd = (opcode >> 11) & 0x1f;
        sa = (opcode >> 6) & 0x1f;

        strcpy(instruction, mips_r_table[n].instr);

        for (r = 0; r < 3; r++)
        {
          if (mips_r_table[n].operand[r] == MIPS_OP_NONE) { break; }

          if (mips_r_table[n].operand[r] == MIPS_OP_RS)
          {
            sprintf(temp, "%s", reg[rs]);
          }
            else
          if (mips_r_table[n].operand[r] == MIPS_OP_RT)
          {
            sprintf(temp, "%s", reg[rt]);
          }
            else
          if (mips_r_table[n].operand[r] == MIPS_OP_RD)
          {
            sprintf(temp, "%s", reg[rd]);
          }
            else
          if (mips_r_table[n].operand[r] == MIPS_OP_SA)
          {
            sprintf(temp, "%d", sa);
          }
            else
          { temp[0] = 0; }

          if (r != 0) { strcat(instruction, ", "); }
          else { strcat(instruction, " "); }

          strcat(instruction, temp);
        }

        break;
      }

      n++;
    }
  }
    else
  if ((opcode >> 27) == 1)
  {
    // J-Type Instruction [ op 6, target 26 ]
    unsigned int upper = (address + 4) & 0xf0000000;
    if ((opcode >> 26) == 2)
    {
      sprintf(instruction, "j 0x%08x", ((opcode & 0x03ffffff) << 2) | upper);
    }
      else
    {
      sprintf(instruction, "jal 0x%08x", ((opcode & 0x03ffffff) << 2) | upper);
    }
  }
    else
  if ((flags & MIPS_EE_VU) && (opcode >> 26) == 0x12)
  {
    disasm_vector(memory, address, instruction, cycles_min, cycles_max);
  }
    else
  {
    int op = opcode >> 26;
    // I-Type?  [ op 6, rs 5, rt 5, imm 16 ]
    n = 0;
    while(mips_i_table[n].instr != NULL)
    {
      // Check of this specific MIPS chip uses this instruction.
      if ((mips_i_table[n].version & flags) == 0)
      {
        n++;
        continue;
      }

      if (mips_i_table[n].function == op)
      {
        rs = (opcode >> 21) & 0x1f;
        rt = (opcode >> 16) & 0x1f;
        immediate = opcode & 0xffff;

        strcpy(instruction, mips_i_table[n].instr);

        for (r = 0; r < 3; r++)
        {
          if (mips_i_table[n].operand[r] == MIPS_OP_NONE) { break; }

          if (mips_i_table[n].operand[r] == MIPS_OP_RS)
          {
            sprintf(temp, "%s", reg[rs]);
          }
            else
          if (mips_i_table[n].operand[r] == MIPS_OP_RT)
          {
            sprintf(temp, "%s", reg[rt]);
          }
            else
          if (mips_i_table[n].operand[r] == MIPS_OP_HINT ||
              mips_i_table[n].operand[r] == MIPS_OP_CACHE)
          {
            sprintf(temp, "%d", rt);
          }
            else
          if (mips_i_table[n].operand[r] == MIPS_OP_FT)
          {
            sprintf(temp, "$f%d", rt);
          }
            else
          if (mips_i_table[n].operand[r] == MIPS_OP_IMMEDIATE)
          {
            sprintf(temp, "0x%x", immediate);
          }
            else
          if (mips_i_table[n].operand[r] == MIPS_OP_IMMEDIATE_SIGNED)
          {
            sprintf(temp, "0x%x (%d)", immediate, (int16_t)immediate);
          }
            else
          if (mips_i_table[n].operand[r] == MIPS_OP_IMMEDIATE_RS)
          {
            sprintf(temp, "0x%x(%s)", immediate, reg[rs]);
          }
            else
          if (mips_i_table[n].operand[r] == MIPS_OP_LABEL)
          {
            int32_t offset = (int16_t)immediate;

            offset = offset << 2;

            sprintf(temp, "0x%x (offset=%d)", address + 4 + offset, offset);
          }
            else
          { temp[0] = 0; }

          if (r != 0) { strcat(instruction, ", "); }
          else { strcat(instruction, " "); }
          strcat(instruction, temp);
        }

        break;
      }

      n++;
    }

    if (mips_i_table[n].instr == NULL)
    {
      //printf("Internal Error: Unknown MIPS opcode %08x, %s:%d\n", opcode, __FILE__, __LINE__);
      strcpy(instruction, "???");
    }
  }

  return 4;
}

void list_output_mips(struct _asm_context *asm_context, uint32_t start, uint32_t end)
{
  int cycles_min,cycles_max;
  char instruction[128];
  uint32_t opcode;

  fprintf(asm_context->list, "\n");

  while(start < end)
  {
    opcode = memory_read32_m(&asm_context->memory, start);

    disasm_mips(&asm_context->memory, asm_context->flags, start, instruction, &cycles_min, &cycles_max);

    fprintf(asm_context->list, "0x%08x: 0x%08x %-40s cycles: ", start, opcode, instruction);

    if (cycles_min == cycles_max)
    { fprintf(asm_context->list, "%d\n", cycles_min); }
      else
    { fprintf(asm_context->list, "%d-%d\n", cycles_min, cycles_max); }

    start += 4;
  }
}

void disasm_range_mips(struct _memory *memory, uint32_t flags, uint32_t start, uint32_t end)
{
  char instruction[128];
  int cycles_min = 0,cycles_max = 0;
  int num;

  printf("\n");

  printf("%-7s %-5s %-40s Cycles\n", "Addr", "Opcode", "Instruction");
  printf("------- ------ ----------------------------------       ------\n");

  while(start < end)
  {
    // FIXME - Need to check endian
    num = READ_RAM(start) |
          (READ_RAM(start + 1) << 8) |
          (READ_RAM(start + 2) << 16) |
          (READ_RAM(start + 3) << 24);

    disasm_mips(memory, flags, start, instruction, &cycles_min, &cycles_max);

    if (cycles_min < 1)
    {
      printf("0x%04x: 0x%08x %-40s ?\n", start, num, instruction);
    }
      else
    if (cycles_min == cycles_max)
    {
      printf("0x%04x: 0x%08x %-40s %d\n", start, num, instruction, cycles_min);
    }
      else
    {
      printf("0x%04x: 0x%08x %-40s %d-%d\n", start, num, instruction, cycles_min, cycles_max);
    }

    start += 4;
  }
}

