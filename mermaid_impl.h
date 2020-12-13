
#pragma once

#include "stdafx.h"
#include "vectorize.h"

// Mermaid/Selkie decompression also happens in two phases, just like in Kraken,
// but the match copier works differently.
// Both Mermaid and Selkie use the same on-disk format, only the compressor
// differs.
typedef struct MermaidLzTable {
  // Flag stream. Format of flags:
  // Read flagbyte from |cmd_stream|
  // If flagbyte >= 24:
  //   flagbyte & 0x80 == 0 : Read from |off16_stream| into |recent_offs|.
  //                   != 0 : Don't read offset.
  //   flagbyte & 7 = Number of literals to copy first from |lit_stream|.
  //   (flagbyte >> 3) & 0xF = Number of bytes to copy from |recent_offs|.
  //
  //  If flagbyte == 0 :
  //    Read byte L from |length_stream|
  //    If L > 251: L += 4 * Read word from |length_stream|
  //    L += 64
  //    Copy L bytes from |lit_stream|.
  //
  //  If flagbyte == 1 :
  //    Read byte L from |length_stream|
  //    If L > 251: L += 4 * Read word from |length_stream|
  //    L += 91
  //    Copy L bytes from match pointed by next offset from |off16_stream|
  //
  //  If flagbyte == 2 :
  //    Read byte L from |length_stream|
  //    If L > 251: L += 4 * Read word from |length_stream|
  //    L += 29
  //    Copy L bytes from match pointed by next offset from |off32_stream|, 
  //    relative to start of block.
  //    Then prefetch |off32_stream[3]|
  //
  //  If flagbyte > 2:
  //    L = flagbyte + 5
  //    Copy L bytes from match pointed by next offset from |off32_stream|,
  //    relative to start of block.
  //    Then prefetch |off32_stream[3]|
  const byte *cmd_stream, *cmd_stream_end;
  
  // Length stream
  const byte *length_stream;

  // Literal stream
  const byte *lit_stream, *lit_stream_end;

  // Near offsets
  const uint16 *off16_stream, *off16_stream_end;

  // Far offsets for current chunk
  uint32 *off32_stream, *off32_stream_end;
  
  // Holds the offsets for the two chunks
  uint32 *off32_stream_1, *off32_stream_2;
  uint32 off32_size_1, off32_size_2;

  // Flag offsets for next 64k chunk.
  uint32 cmd_stream_2_offs, cmd_stream_2_offs_end;
} MermaidLzTable;

//VECTORIZED is a macro that, when using GNU C, sets an attribute that tells the compiler
//to generate multiple versions targeting different platforms (which are folded if result is identical)

int Mermaid_DecodeFarOffsets(const byte *src, const byte *src_end, uint32 *output, size_t output_size, int64 offset);

VECTORIZED bool Mermaid_ReadLzTable(int mode,
                         const byte *src, const byte *src_end,
                         byte *dst, int dst_size, int64 offset,
                         byte *scratch, byte *scratch_end, MermaidLzTable *lz);
const byte *Mermaid_Mode0(byte *dst, size_t dst_size, byte *dst_ptr_end, byte *dst_start,
                          const byte *src_end, MermaidLzTable *lz, int32 *saved_dist, size_t startoff);
const byte *Mermaid_Mode1(byte *dst, size_t dst_size, byte *dst_ptr_end, byte *dst_start,
                         const byte *src_end, MermaidLzTable *lz, int32 *saved_dist, size_t startoff);
bool Mermaid_ProcessLzRuns(int mode,
                           const byte *src, const byte *src_end,
                           byte *dst, size_t dst_size, uint64 offset, byte *dst_end,
                           MermaidLzTable *lz);
int Mermaid_DecodeQuantum(byte *dst, byte *dst_end, byte *dst_start,
                          const byte *src, const byte *src_end,
                          byte *temp, byte *temp_end);
