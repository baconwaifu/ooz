#pragma once

#include "stdafx.h"
#include "vectorize.h"

// The Kraken structure includes.
#include "kraken.h"
#include "bitreader.h"

struct NewHuffLut {
  // Mapping that maps a bit pattern to a code length.
  uint8 bits2len[2048 + 16];
  // Mapping that maps a bit pattern to a symbol.
  uint8 bits2sym[2048 + 16];
};

struct HuffRange {
  uint16 symbol;
  uint16 num;
};

struct HuffRevLut {
  uint8 bits2len[2048];
  uint8 bits2sym[2048];
};

struct TansLutEnt {
  uint32 x;
  uint8 bits_x;
  uint8 symbol;
  uint16 w;
};

typedef struct HuffReader {
  // Array to hold the output of the huffman read array operation
  byte *output, *output_end;
  // We decode three parallel streams, two forwards, |src| and |src_mid|
  // while |src_end| is decoded backwards. 
  const byte *src, *src_mid, *src_end, *src_mid_org;
  int src_bitpos, src_mid_bitpos, src_end_bitpos;
  uint32 src_bits, src_mid_bits, src_end_bits;
} HuffReader;

struct TansDecoderParams {
  TansLutEnt *lut;
  uint8 *dst, *dst_end;
  const uint8 *ptr_f, *ptr_b;
  uint32 bits_f, bits_b;
  int bitpos_f, bitpos_b;
  uint32 state_0, state_1, state_2, state_3, state_4;
};

struct TansData {
  uint32 A_used;
  uint32 B_used;
  uint8 A[256];
  uint32 B[256];
};

int Kraken_DecodeBytes(byte **output, const byte *src, const byte *src_end, int *decoded_size, size_t output_size, bool force_memmove, uint8 *scratch, uint8 *scratch_end);
int Kraken_GetBlockSize(const uint8 *src, const uint8 *src_end, int *dest_size, int dest_capacity);
int Huff_ConvertToRanges(HuffRange *range, int num_symbols, int P, const uint8 *symlen, BitReader *bits);
int Kraken_Decompress(const byte *src, size_t src_len, byte *dst, size_t dst_len);
bool Kraken_DecodeStep(struct KrakenDecoder *dec,
                       byte *dst_start, int offset, size_t dst_bytes_left_in,
                       const byte *src, size_t src_bytes_left);
uint32 BSF(uint32 x);
uint32 BSR(uint32 x);
int CountLeadingZeros(uint32 bits);
int Log2RoundUp(uint32 v);
bool Kraken_DecodeBytesCore(HuffReader *hr, HuffRevLut *lut);
int Huff_ReadCodeLengthsOld(BitReader *bits, uint8 *syms, uint32 *code_prefix);
bool DecodeGolombRiceLengths(uint8 *dst, size_t size, BitReader2 *br);
bool DecodeGolombRiceBits(uint8 *dst, uint size, uint bitcount, BitReader2 *br);
int Huff_ReadCodeLengthsNew(BitReader *bits, uint8 *syms, uint32 *code_prefix);
int Huff_ConvertToRanges(HuffRange *range, int num_symbols, int P, const uint8 *symlen, BitReader *bits);
bool Huff_MakeLut(const uint32 *prefix_org, const uint32 *prefix_cur, NewHuffLut *hufflut, uint8 *syms);
void FillByteOverflow16(uint8 *dst, uint8 v, size_t n);
void Kraken_CopyWholeMatch(byte *dst, uint32 offset, size_t length);
int Kraken_DecodeQuantum(byte *dst, byte *dst_end, byte *dst_start,
                         const byte *src, const byte *src_end,
                         byte *scratch, byte *scratch_end);
bool Kraken_ProcessLzRuns(int mode, byte *dst, int dst_size, int offset, KrakenLzTable *lztable);
bool Kraken_ProcessLzRuns_Type1(KrakenLzTable *lzt, byte *dst, byte *dst_end, byte *dst_start);
bool Kraken_ProcessLzRuns_Type0(KrakenLzTable *lzt, byte *dst, byte *dst_end, byte *dst_start);
bool Kraken_ReadLzTable(int mode,
                        const byte *src, const byte *src_end,
                        byte *dst, int dst_size, int offset,
                        byte *scratch, byte *scratch_end, KrakenLzTable *lztable);
void Tans_InitLut(TansData *tans_data, int L_bits, TansLutEnt *lut);
bool Kraken_UnpackOffsets(const byte *src, const byte *src_end,
                          const byte *packed_offs_stream, const byte *packed_offs_stream_extra, int packed_offs_stream_size,
                          int multi_dist_scale,
                          const byte *packed_litlen_stream, int packed_litlen_stream_size,
                          int *offs_stream, int *len_stream,
                          bool excess_flag, UNUSED int excess_bytes);
void CombineScaledOffsetArrays(int *offs_stream, size_t offs_stream_size, int scale, const uint8 *low_bits);
int Krak_DecodeTans(const byte *src, size_t src_size, byte *dst, int dst_size, uint8 *scratch, uint8 *scratch_end);
bool Tans_Decode(TansDecoderParams *params);
bool Tans_DecodeTable(BitReader *bits, int L_bits, TansData *tans_data);
template<typename T> void SimpleSort(T *p, T *pend);
int Krak_DecodeRLE(const byte *src, size_t src_size, byte *dst, int dst_size, uint8 *scratch, uint8 *scratch_end);
int Krak_DecodeRecursive(const byte *src, size_t src_size, byte *output, int output_size, uint8 *scratch, uint8 *scratch_end);
int Kraken_DecodeMultiArray(const uint8 *src, const uint8 *src_end,
                            uint8 *dst, uint8 *dst_end,
                            uint8 **array_data, int *array_lens, int array_count,
                            int *total_size_out, bool force_memmove, uint8 *scratch, uint8 *scratch_end);
int Kraken_DecodeBytes_Type12(const byte *src, size_t src_size, byte *output, int output_size, int type);
uint32 Kraken_GetCrc(UNUSED const byte *p, UNUSED size_t p_size);
const byte *Kraken_ParseQuantumHeader(KrakenQuantumHeader *hdr, const byte *p, bool use_checksum);
const byte *Kraken_ParseHeader(KrakenHeader *hdr, const byte *p);
void Kraken_Destroy(KrakenDecoder *kraken);
KrakenDecoder *Kraken_Create();
