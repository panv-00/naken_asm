/**
 *  naken_asm assembler.
 *  Author: Michael Kohn
 *   Email: mike@mikekohn.net
 *     Web: https://www.mikekohn.net/
 * License: GPLv3
 *
 * Copyright 2010-2022 by Michael Kohn
 *
 */

#ifndef NAKEN_ASM_DISASM_ARC_H
#define NAKEN_ASM_DISASM_ARC_H

#include "common/assembler.h"

int disasm_arc(
  struct _memory *memory,
  uint32_t address,
  char *instruction,
  int *cycles_min,
  int *cycles_max);

void list_output_arc(
  struct _asm_context *asm_context,
  uint32_t start,
  uint32_t end);

void disasm_range_arc(
  struct _memory *memory,
  uint32_t flags,
  uint32_t start,
  uint32_t end);

#endif

