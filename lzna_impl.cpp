
#include "lzna_impl.h"

const byte *LZNA_ParseWholeMatchInfo(const byte *p, uint32 *dist) {
  uint32 v = _byteswap_ushort(*(uint16*)p);

  if (v < 0x8000) {
    uint32 x = 0, b, pos = 0;
    for (;;) {
      b = p[2];
      p += 1;
      if (b & 0x80)
        break;
      x += (b + 0x80) << pos;
      pos += 7;

    }
    x += (b - 128) << pos;
    *dist = 0x8000 + v + (x << 15) + 1;
    return p + 2;
  } else {
    *dist = v - 0x8000 + 1;
    return p + 2;
  }
}

const byte *LZNA_ParseQuantumHeader(KrakenQuantumHeader *hdr, const byte *p, bool use_checksum, int raw_len) {
  uint32 v = (p[0] << 8) | p[1];
  uint32 size = v & 0x3FFF;
  if (size != 0x3fff) {
    hdr->compressed_size = size + 1;
    hdr->flag1 = (v >> 14) & 1;
    hdr->flag2 = (v >> 15) & 1;
    if (use_checksum) {
      hdr->checksum = (p[2] << 16) | (p[3] << 8) | p[4];
      return p + 5;
    } else {
      return p + 2;
    }
  }
  v >>= 14;
  if (v == 0) {
    p = LZNA_ParseWholeMatchInfo(p + 2, &hdr->whole_match_distance);
    hdr->compressed_size = 0;
    return p;
  }
  if (v == 1) {
    // memset
    hdr->checksum = p[2];
    hdr->compressed_size = 0;
    hdr->whole_match_distance = 0;
    return p + 3;
  }
  if (v == 2) {
    // uncompressed
    hdr->compressed_size = raw_len;
    return p + 2;
  }
  return NULL;
}

