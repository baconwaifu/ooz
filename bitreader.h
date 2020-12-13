#pragma once

#include "stdafx.h"
#include "vectorize.h"

struct BitReader2 {
  const uint8 *p, *p_end;
  uint32 bitpos;
};

typedef struct BitReader {
  // |p| holds the current byte and |p_end| the end of the buffer.
  const byte *p, *p_end;
  // Bits accumulated so far
  uint32 bits;
  // Next byte will end up in the |bitpos| position in |bits|.
  int bitpos;
} BitReader;

bool BitReader_ReadLengthB(BitReader *bits, uint32 *v);
bool BitReader_ReadLength(BitReader *bits, uint32 *v);
uint32 BitReader_ReadDistanceB(BitReader *bits, uint32 v);
uint32 BitReader_ReadDistance(BitReader *bits, uint32 v);
int BitReader_ReadGammaX(BitReader *bits, int forced);
int BitReader_ReadGamma(BitReader *bits);
uint32 BitReader_ReadMoreThan24BitsB(BitReader *bits, int n);
uint32 BitReader_ReadMoreThan24Bits(BitReader *bits, int n);
int BitReader_ReadBitsNoRefillZero(BitReader *bits, int n);
int BitReader_ReadBitsNoRefill(BitReader *bits, int n);
int BitReader_ReadBitNoRefill(BitReader *bits);
int BitReader_ReadBit(BitReader *bits);
void BitReader_RefillBackwards(BitReader *bits);
void BitReader_Refill(BitReader *bits);
int BitReader_ReadFluff(BitReader *bits, int num_symbols);
