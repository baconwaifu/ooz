#pragma once

#include "stdafx.h"
// Provides vector primitives with fallbacks
#include "vectorize.h"

struct LeviathanLzTable {
  int *offs_stream;
  int offs_stream_size;
  int *len_stream;
  int len_stream_size;
  uint8 *lit_stream[16];
  int lit_stream_size[16];
  int lit_stream_total;
  uint8 *multi_cmd_ptr[8];
  uint8 *multi_cmd_end[8];
  uint8 *cmd_stream;
  int cmd_stream_size;
};

struct LeviathanModeRaw;
struct LeviathanModeSub;
struct LeviathanModeLamSub;
struct LeviathanModeSubAnd3;
struct LeviathanModeSubAndF;
struct LeviathanModeO1;

bool Leviathan_ReadLzTable(int chunk_type,
                           const byte *src, const byte *src_end,
                           byte *dst, int dst_size, int offset,
                           byte *scratch, byte *scratch_end, LeviathanLzTable *lztable);
template<typename Mode, bool MultiCmd>
bool Leviathan_ProcessLz(LeviathanLzTable *lzt, uint8 *dst,
                         uint8 *dst_start, uint8 *dst_end, uint8 *window_base);
bool Leviathan_ProcessLzRuns(int chunk_type, byte *dst, int dst_size, int offset, LeviathanLzTable *lzt);
int Leviathan_DecodeQuantum(byte *dst, byte *dst_end, byte *dst_start,
                            const byte *src, const byte *src_end,
                            byte *scratch, byte *scratch_end);

