
#include "bitreader.h"

int BitReader_ReadFluff(BitReader *bits, int num_symbols) {
  unsigned long y;

  if (num_symbols == 256)
    return 0;

  int x = 257 - num_symbols;
  if (x > num_symbols)
    x = num_symbols;

  x *= 2;

  _BitScanReverse(&y, x - 1);
  y += 1;

  uint32 v = bits->bits >> (32 - y);
  uint32 z = (1 << y) - x;

  if ((v >> 1) >= z) {
    bits->bits <<= y;
    bits->bitpos += y;
    return v - z;
  } else {
    bits->bits <<= (y - 1);
    bits->bitpos += (y - 1);
    return (v >> 1);
  }
}

// Read more bytes to make sure we always have at least 24 bits in |bits|.
void BitReader_Refill(BitReader *bits) {
  assert(bits->bitpos <= 24);
  while (bits->bitpos > 0) {
    bits->bits |= (bits->p < bits->p_end ? *bits->p : 0) << bits->bitpos;
    bits->bitpos -= 8;
    bits->p++;
  }
}

// Read more bytes to make sure we always have at least 24 bits in |bits|,
// used when reading backwards.
void BitReader_RefillBackwards(BitReader *bits) {
  assert(bits->bitpos <= 24);
  while (bits->bitpos > 0) {
    bits->p--;
    bits->bits |= (bits->p >= bits->p_end ? *bits->p : 0) << bits->bitpos;
    bits->bitpos -= 8;
  }
}

// Refill bits then read a single bit.
int BitReader_ReadBit(BitReader *bits) {
  int r;
  BitReader_Refill(bits);
  r = bits->bits >> 31;
  bits->bits <<= 1;
  bits->bitpos += 1;
  return r;
}

int BitReader_ReadBitNoRefill(BitReader *bits) {
  int r;
  r = bits->bits >> 31;
  bits->bits <<= 1;
  bits->bitpos += 1;
  return r;
}


// Read |n| bits without refilling.
int BitReader_ReadBitsNoRefill(BitReader *bits, int n) {
  int r = (bits->bits >> (32 - n));
  bits->bits <<= n;
  bits->bitpos += n;
  return r;
}

// Read |n| bits without refilling, n may be zero.
int BitReader_ReadBitsNoRefillZero(BitReader *bits, int n) {
  int r = (bits->bits >> 1 >> (31 - n));
  bits->bits <<= n;
  bits->bitpos += n;
  return r;
}

uint32 BitReader_ReadMoreThan24Bits(BitReader *bits, int n) {
  uint32 rv;
  if (n <= 24) {
    rv = BitReader_ReadBitsNoRefillZero(bits, n);
  } else {
    rv = BitReader_ReadBitsNoRefill(bits, 24) << (n - 24);
    BitReader_Refill(bits);
    rv += BitReader_ReadBitsNoRefill(bits, n - 24);
  }
  BitReader_Refill(bits);
  return rv;
}

uint32 BitReader_ReadMoreThan24BitsB(BitReader *bits, int n) {
  uint32 rv;
  if (n <= 24) {
    rv = BitReader_ReadBitsNoRefillZero(bits, n);
  } else {
    rv = BitReader_ReadBitsNoRefill(bits, 24) << (n - 24);
    BitReader_RefillBackwards(bits);
    rv += BitReader_ReadBitsNoRefill(bits, n - 24);
  }
  BitReader_RefillBackwards(bits);
  return rv;
}

// Reads a gamma value.
// Assumes bitreader is already filled with at least 23 bits
int BitReader_ReadGamma(BitReader *bits) {
  unsigned long bitresult;
  int n;
  int r;
  if (bits->bits != 0) {
    _BitScanReverse(&bitresult, bits->bits);
    n = 31 - bitresult;
  } else {
    n = 32;
  }
  n = 2 * n + 2;
  assert(n < 24);
  bits->bitpos += n;
  r = bits->bits >> (32 - n);
  bits->bits <<= n;
  return r - 2;
}

// Reads a gamma value with |forced| number of forced bits.
int BitReader_ReadGammaX(BitReader *bits, int forced) {
  unsigned long bitresult;
  int r;
  if (bits->bits != 0) {
    _BitScanReverse(&bitresult, bits->bits);
    int lz = 31 - bitresult;
    assert(lz < 24);
    r = (bits->bits >> (31 - lz - forced)) + ((lz - 1) << forced);
    bits->bits <<= lz + forced + 1;
    bits->bitpos += lz + forced + 1;
    return r;
  }
  return 0;
}

// Reads a offset code parametrized by |v|.
uint32 BitReader_ReadDistance(BitReader *bits, uint32 v) {
  uint32 w, m, n, rv;
  if (v < 0xF0) {
    n = (v >> 4) + 4;
    w = _rotl(bits->bits | 1, n);
    bits->bitpos += n;
    m = (2 << n) - 1;
    bits->bits = w & ~m;
    rv = ((w & m) << 4) + (v & 0xF) - 248;
  } else {
    n = v - 0xF0 + 4;
    w = _rotl(bits->bits | 1, n);
    bits->bitpos += n;
    m = (2 << n) - 1;
    bits->bits = w & ~m;
    rv = 8322816 + ((w & m) << 12);
    BitReader_Refill(bits);
    rv += (bits->bits >> 20);
    bits->bitpos += 12;
    bits->bits <<= 12;
  }
  BitReader_Refill(bits);
  return rv;
}


// Reads a offset code parametrized by |v|, backwards.
uint32 BitReader_ReadDistanceB(BitReader *bits, uint32 v) {
  uint32 w, m, n, rv;
  if (v < 0xF0) {
    n = (v >> 4) + 4;
    w = _rotl(bits->bits | 1, n);
    bits->bitpos += n;
    m = (2 << n) - 1;
    bits->bits = w & ~m;
    rv = ((w & m) << 4) + (v & 0xF) - 248;
  } else {
    n = v - 0xF0 + 4;
    w = _rotl(bits->bits | 1, n);
    bits->bitpos += n;
    m = (2 << n) - 1;
    bits->bits = w & ~m;
    rv = 8322816 + ((w & m) << 12);
    BitReader_RefillBackwards(bits);
    rv += (bits->bits >> (32 - 12));
    bits->bitpos += 12;
    bits->bits <<= 12;
  }
  BitReader_RefillBackwards(bits);
  return rv;
}

// Reads a length code.
bool BitReader_ReadLength(BitReader *bits, uint32 *v) {
  unsigned long bitresult;
  int n;
  uint32 rv;
  _BitScanReverse(&bitresult, bits->bits);
  n = 31 - bitresult;
  if (n > 12) return false;
  bits->bitpos += n;
  bits->bits <<= n;
  BitReader_Refill(bits);
  n += 7;
  bits->bitpos += n;
  rv = (bits->bits >> (32 - n)) - 64;
  bits->bits <<= n;
  *v = rv;
  BitReader_Refill(bits);
  return true;
}

// Reads a length code, backwards.
bool BitReader_ReadLengthB(BitReader *bits, uint32 *v) {
  unsigned long bitresult;
  int n;
  uint32 rv;
  _BitScanReverse(&bitresult, bits->bits);
  n = 31 - bitresult;
  if (n > 12) return false;
  bits->bitpos += n;
  bits->bits <<= n;
  BitReader_RefillBackwards(bits);
  n += 7;
  bits->bitpos += n;
  rv = (bits->bits >> (32 - n)) - 64;
  bits->bits <<= n;
  *v = rv;
  BitReader_RefillBackwards(bits);
  return true;
}


