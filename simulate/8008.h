/**
 *  naken_asm assembler.
 *  Author: Michael Kohn
 *   Email: mike@mikekohn.net
 *     Web: http://www.mikekohn.net/
 * License: GPLv3
 *
 * Copyright 2010-2019 by Michael Kohn
 *
 */

#ifndef NAKEN_ASM__SIMULATE_8008_H
#define NAKEN_ASM__SIMULATE_8008_H

#include <unistd.h>

#include "common/memory.h"
#include "simulate/common.h"

struct _simulate_8008
{
  uint8_t reg_a;                       // 8-bit Accumulator
  uint8_t reg_b, reg_c, reg_d, reg_e;  // 8-bit Registers
  uint8_t reg_hi, reg_lo;              // 8-bit Hi/Lo Registers
  uint16_t reg_st[8];                  // 14-bit Stack (st[0] -> Prog. Counter)
  uint8_t reg_sp;                      // Stack Pointer [0 - 7]
};

struct _simulate *simulate_init_8008();
void simulate_free_8008(struct _simulate *simulate);
int simulate_dumpram_8008(struct _simulate *simulate, int start, int end);
void simulate_push_8008(struct _simulate *simulate, uint8_t value);
int simulate_set_reg_8008(struct _simulate *simulate, char *reg_string, uint32_t value);
uint32_t simulate_get_reg_8008(struct _simulate *simulate, char *reg_string);
void simulate_set_pc_8008(struct _simulate *simulate, uint32_t value);
void simulate_reset_8008(struct _simulate *simulate);
void simulate_dump_registers_8008(struct _simulate *simulate);
int simulate_run_8008(struct _simulate *simulate, int max_cycles, int step);

#endif

