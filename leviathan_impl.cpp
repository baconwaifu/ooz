
#include "leviathan_impl.h"
#include "kraken_impl.h"
#include "util.h"

#ifdef __GNUC__
#define finline __attribute__((always_inline))
#else
#define finline __forceinline
#endif

bool Leviathan_ReadLzTable(int chunk_type,
                           const byte *src, const byte *src_end,
                           byte *dst, int dst_size, int offset,
                           byte *scratch, byte *scratch_end, LeviathanLzTable *lztable) {
  byte *packed_offs_stream, *packed_len_stream, *out;
  int decode_count, n;

  if (chunk_type > 5)
    return false;

  if (src_end - src < 13)
    return false;

  if (offset == 0) {
    COPY_64(dst, src);
    dst += 8;
    src += 8;
  }

  int offs_scaling = 0;
  uint8 *packed_offs_stream_extra = NULL;


  int offs_stream_limit = dst_size / 3;

  if (!(src[0] & 0x80)) {
    // Decode packed offset stream, it's bounded by the command length.
    packed_offs_stream = scratch;
    n = Kraken_DecodeBytes(&packed_offs_stream, src, src_end, &lztable->offs_stream_size,
                           Min(scratch_end - scratch, offs_stream_limit), false, scratch, scratch_end);
    if (n < 0)
      return false;
    src += n;
    scratch += lztable->offs_stream_size;
  } else {
    // uses the mode where distances are coded with 2 tables
    // and the transformation offs * scaling + low_bits
    offs_scaling = src[0] - 127;
    src++;

    packed_offs_stream = scratch;
    n = Kraken_DecodeBytes(&packed_offs_stream, src, src_end, &lztable->offs_stream_size,
                           Min(scratch_end - scratch, offs_stream_limit), false, scratch, scratch_end);
    if (n < 0)
      return false;
    src += n;
    scratch += lztable->offs_stream_size;

    if (offs_scaling != 1) {
      packed_offs_stream_extra = scratch;
      n = Kraken_DecodeBytes(&packed_offs_stream_extra, src, src_end, &decode_count,
                             Min(scratch_end - scratch, offs_stream_limit), false, scratch, scratch_end);
      if (n < 0 || decode_count != lztable->offs_stream_size)
        return false;
      src += n;
      scratch += decode_count;
    }
  }

  // Decode packed litlen stream. It's bounded by 1/5 of dst_size.
  packed_len_stream = scratch;
  n = Kraken_DecodeBytes(&packed_len_stream, src, src_end, &lztable->len_stream_size,
                         Min(scratch_end - scratch, dst_size / 5), false, scratch, scratch_end);
  if (n < 0)
    return false;
  src += n;
  scratch += lztable->len_stream_size;

  // Reserve memory for final dist stream
  scratch = ALIGN_POINTER(scratch, 16);
  lztable->offs_stream = (int*)scratch;
  scratch += lztable->offs_stream_size * 4;

  // Reserve memory for final len stream
  scratch = ALIGN_POINTER(scratch, 16);
  lztable->len_stream = (int*)scratch;
  scratch += lztable->len_stream_size * 4;

  if (scratch > scratch_end)
    return false;

  if (chunk_type <= 1) {
    // Decode lit stream, bounded by dst_size
    out = scratch;
    n = Kraken_DecodeBytes(&out, src, src_end, &decode_count, Min(scratch_end - scratch, dst_size),
                           true, scratch, scratch_end);
    if (n < 0)
      return false;
    src += n;
    lztable->lit_stream[0] = out;
    lztable->lit_stream_size[0] = decode_count;
  } else {
    int array_count = (chunk_type == 2) ? 2 :
                      (chunk_type == 3) ? 4 : 16;
    n = Kraken_DecodeMultiArray(src, src_end, scratch, scratch_end, lztable->lit_stream,
                                lztable->lit_stream_size, array_count, &decode_count,
                                true, scratch, scratch_end);
    if (n < 0)
      return false;
    src += n;
  }
  scratch += decode_count;
  lztable->lit_stream_total = decode_count;

  if (src >= src_end)
    return false;

  if (!(src[0] & 0x80)) {
    // Decode command stream, bounded by dst_size
    out = scratch;
    n = Kraken_DecodeBytes(&out, src, src_end, &decode_count, Min(scratch_end - scratch, dst_size),
                           true, scratch, scratch_end);
    if (n < 0)
      return false;
    src += n;
    lztable->cmd_stream = out;
    lztable->cmd_stream_size = decode_count;
    scratch += decode_count;
  } else {
    if (src[0] != 0x83)
      return false;
    src++;
    int multi_cmd_lens[8];
    n = Kraken_DecodeMultiArray(src, src_end, scratch, scratch_end, lztable->multi_cmd_ptr,
                                multi_cmd_lens, 8, &decode_count, true, scratch, scratch_end);
    if (n < 0)
      return false;
    src += n;
    for (size_t i = 0; i < 8; i++)
      lztable->multi_cmd_end[i] = lztable->multi_cmd_ptr[i] + multi_cmd_lens[i];

    lztable->cmd_stream = NULL;
    lztable->cmd_stream_size = decode_count;
    scratch += decode_count;
  }

  if (dst_size > scratch_end - scratch)
    return false;


  return Kraken_UnpackOffsets(src, src_end, packed_offs_stream, packed_offs_stream_extra,
                              lztable->offs_stream_size, offs_scaling,
                              packed_len_stream, lztable->len_stream_size,
                              lztable->offs_stream, lztable->len_stream, 0, 0);
}


struct LeviathanModeRaw {
  const uint8 *lit_stream;

  finline LeviathanModeRaw(LeviathanLzTable *lzt, uint8 *dst_start) : lit_stream(lzt->lit_stream[0]) {
  }
  
  finline bool CopyLiterals(uint32 cmd, uint8 *&dst, const int *&len_stream, uint8 *match_zone_end, size_t last_offset) {
    uint32 litlen = (cmd >> 3) & 3;
    // use cmov
    uint32 len_stream_value = *len_stream & 0xffffff;
    const int *next_len_stream = len_stream + 1;
    len_stream = (litlen == 3) ? next_len_stream : len_stream;
    litlen = (litlen == 3) ? len_stream_value : litlen;
    COPY_64(dst, lit_stream);
    if (litlen > 8) {
      COPY_64(dst + 8, lit_stream + 8);
      if (litlen > 16) {
        COPY_64(dst + 16, lit_stream + 16);
        if (litlen > 24) {
          if (litlen > match_zone_end - dst)
            return false;  // out of bounds
          do {
            COPY_64(dst + 24, lit_stream + 24);
            litlen -= 8, dst += 8, lit_stream += 8;
          } while (litlen > 24);
        }
      }
    }
    dst += litlen;
    lit_stream += litlen;
    return true;
  }

  finline void CopyFinalLiterals(uint32 final_len, uint8 *&dst, size_t last_offset) {
    if (final_len >= 64) {
      do {
        COPY_64_BYTES(dst, lit_stream);
        dst += 64, lit_stream += 64, final_len -= 64;
      } while (final_len >= 64);
    }
    if (final_len >= 8) {
      do {
        COPY_64(dst, lit_stream);
        dst += 8, lit_stream += 8, final_len -= 8;
      } while (final_len >= 8);
    }
    if (final_len > 0) {
      do {
        *dst++ = *lit_stream++;
      } while (--final_len);
    }
  }
};

struct LeviathanModeSub {
  const uint8 *lit_stream;

  finline LeviathanModeSub(LeviathanLzTable *lzt, uint8 *dst_start) : lit_stream(lzt->lit_stream[0]) {
  }

  finline bool CopyLiterals(uint32 cmd, uint8 *&dst, const int *&len_stream, uint8 *match_zone_end, size_t last_offset) {
    uint32 litlen = (cmd >> 3) & 3;
    // use cmov
    uint32 len_stream_value = *len_stream & 0xffffff;
    const int *next_len_stream = len_stream + 1;
    len_stream = (litlen == 3) ? next_len_stream : len_stream;
    litlen = (litlen == 3) ? len_stream_value : litlen;
    COPY_64_ADD(dst, lit_stream, &dst[last_offset]);
    if (litlen > 8) {
      COPY_64_ADD(dst + 8, lit_stream + 8, &dst[last_offset + 8]);
      if (litlen > 16) {
        COPY_64_ADD(dst + 16, lit_stream + 16, &dst[last_offset + 16]);
        if (litlen > 24) {
          if (litlen > match_zone_end - dst)
            return false;  // out of bounds
          do {
            COPY_64_ADD(dst + 24, lit_stream + 24, &dst[last_offset + 24]);
            litlen -= 8, dst += 8, lit_stream += 8;
          } while (litlen > 24);
        }
      }
    }
    dst += litlen;
    lit_stream += litlen;
    return true;
  }

  finline void CopyFinalLiterals(uint32 final_len, uint8 *&dst, size_t last_offset) {
    if (final_len >= 8) {
      do {
        COPY_64_ADD(dst, lit_stream, &dst[last_offset]);
        dst += 8, lit_stream += 8, final_len -= 8;
      } while (final_len >= 8);
    }
    if (final_len > 0) {
      do {
        *dst = *lit_stream++ + dst[last_offset];
      } while (dst++, --final_len);
    }
  }
};

struct LeviathanModeLamSub {
  const uint8 *lit_stream, *lam_lit_stream;

  finline LeviathanModeLamSub(LeviathanLzTable *lzt, uint8 *dst_start) 
    : lit_stream(lzt->lit_stream[0]),
      lam_lit_stream(lzt->lit_stream[1]) {
  }

  finline bool CopyLiterals(uint32 cmd, uint8 *&dst, const int *&len_stream, uint8 *match_zone_end, size_t last_offset) {
    uint32 lit_cmd = cmd & 0x18;
    if (!lit_cmd)
      return true;

    uint32 litlen = lit_cmd >> 3;
    // use cmov
    uint32 len_stream_value = *len_stream & 0xffffff;
    const int *next_len_stream = len_stream + 1;
    len_stream = (litlen == 3) ? next_len_stream : len_stream;
    litlen = (litlen == 3) ? len_stream_value : litlen;
       
    if (litlen-- == 0)
      return false; // lamsub mode requires one literal

    dst[0] = *lam_lit_stream++ + dst[last_offset], dst++;

    COPY_64_ADD(dst, lit_stream, &dst[last_offset]);
    if (litlen > 8) {
      COPY_64_ADD(dst + 8, lit_stream + 8, &dst[last_offset + 8]);
      if (litlen > 16) {
        COPY_64_ADD(dst + 16, lit_stream + 16, &dst[last_offset + 16]);
        if (litlen > 24) {
          if (litlen > match_zone_end - dst)
            return false;  // out of bounds
          do {
            COPY_64_ADD(dst + 24, lit_stream + 24, &dst[last_offset + 24]);
            litlen -= 8, dst += 8, lit_stream += 8;
          } while (litlen > 24);
        }
      }
    }
    dst += litlen;
    lit_stream += litlen;
    return true;
  }

  finline void CopyFinalLiterals(uint32 final_len, uint8 *&dst, size_t last_offset) {
    dst[0] = *lam_lit_stream++ + dst[last_offset], dst++;
    final_len -= 1;

    if (final_len >= 8) {
      do {
        COPY_64_ADD(dst, lit_stream, &dst[last_offset]);
        dst += 8, lit_stream += 8, final_len -= 8;
      } while (final_len >= 8);
    }
    if (final_len > 0) {
      do {
        *dst = *lit_stream++ + dst[last_offset];
      } while (dst++, --final_len);
    }
  }
};

struct LeviathanModeSubAnd3 {
  enum { NUM = 4, MASK = NUM - 1};
  const uint8 *lit_stream[NUM];

  finline LeviathanModeSubAnd3(LeviathanLzTable *lzt, uint8 *dst_start) {
    for (size_t i = 0; i != NUM; i++)
      lit_stream[i] = lzt->lit_stream[(-(intptr_t)dst_start + i) & MASK];
  }
  finline bool CopyLiterals(uint32 cmd, uint8 *&dst, const int *&len_stream, uint8 *match_zone_end, size_t last_offset) {
    uint32 lit_cmd = cmd & 0x18;

    if (lit_cmd == 0x18) {
      uint32 litlen = *len_stream++ & 0xffffff;
      if (litlen > match_zone_end - dst)
        return false;
      while (litlen) {
        *dst = *lit_stream[(uintptr_t)dst & MASK]++ + dst[last_offset];
        dst++, litlen--;
      }
    } else if (lit_cmd) {
      *dst = *lit_stream[(uintptr_t)dst & MASK]++ + dst[last_offset];
      dst++;
      if (lit_cmd == 0x10) {
        *dst = *lit_stream[(uintptr_t)dst & MASK]++ + dst[last_offset];
        dst++;
      }
    }
    return true;
  }

  finline void CopyFinalLiterals(uint32 final_len, uint8 *&dst, size_t last_offset) {
    if (final_len > 0) {
      do {
        *dst = *lit_stream[(uintptr_t)dst & MASK]++ + dst[last_offset];
      } while (dst++, --final_len);
    }
  }
};

struct LeviathanModeSubAndF {
  enum { NUM = 16, MASK = NUM - 1};
  const uint8 *lit_stream[NUM];
  
  finline LeviathanModeSubAndF(LeviathanLzTable *lzt, uint8 *dst_start) {
    for(size_t i = 0; i != NUM; i++)
      lit_stream[i] = lzt->lit_stream[(-(intptr_t)dst_start + i) & MASK];
  }
  finline bool CopyLiterals(uint32 cmd, uint8 *&dst, const int *&len_stream, uint8 *match_zone_end, size_t last_offset) {
    uint32 lit_cmd = cmd & 0x18;

    if (lit_cmd == 0x18) {
      uint32 litlen = *len_stream++ & 0xffffff;
      if (litlen > match_zone_end - dst)
        return false;
      while (litlen) {
        *dst = *lit_stream[(uintptr_t)dst & MASK]++ + dst[last_offset];
        dst++, litlen--;
      }
    } else if (lit_cmd) {
      *dst = *lit_stream[(uintptr_t)dst & MASK]++ + dst[last_offset];
      dst++;
      if (lit_cmd == 0x10) {
        *dst = *lit_stream[(uintptr_t)dst & MASK]++ + dst[last_offset];
        dst++;
      }
    }
    return true;
  }

  finline void CopyFinalLiterals(uint32 final_len, uint8 *&dst, size_t last_offset) {
    if (final_len > 0) {
      do {
        *dst = *lit_stream[(uintptr_t)dst & MASK]++ + dst[last_offset];
      } while (dst++, --final_len);
    }
  }
};

struct LeviathanModeO1 {
  const uint8 *lit_streams[16];
  uint8 next_lit[16];
  
  finline LeviathanModeO1(LeviathanLzTable *lzt, uint8 *dst_start) {
    for (size_t i = 0; i != 16; i++) {
      uint8 *p = lzt->lit_stream[i];
      next_lit[i] = *p;
      lit_streams[i] = p + 1;
    }
  }

  finline bool CopyLiterals(uint32 cmd, uint8 *&dst, const int *&len_stream, uint8 *match_zone_end, size_t last_offset) {
    uint32 lit_cmd = cmd & 0x18;

    if (lit_cmd == 0x18) {
      uint32 litlen = *len_stream++;
      if ((int32)litlen <= 0)
        return false;
      uint context = dst[-1];
      do {
        size_t slot = context >> 4;
        *dst++ = (context = next_lit[slot]);
        next_lit[slot] = *lit_streams[slot]++;
      } while (--litlen);
    } else if (lit_cmd) {
      // either 1 or 2
      uint context = dst[-1];
      size_t slot = context >> 4;
      *dst++ = (context = next_lit[slot]);
      next_lit[slot] = *lit_streams[slot]++;
      if (lit_cmd == 0x10) {
        slot = context >> 4;
        *dst++ = (context = next_lit[slot]);
        next_lit[slot] = *lit_streams[slot]++;
      }
    }
    return true;
  }

  finline void CopyFinalLiterals(uint32 final_len, uint8 *&dst, size_t last_offset) {
    uint context = dst[-1];
    while (final_len) {
      size_t slot = context >> 4;
      *dst++ = (context = next_lit[slot]);
      next_lit[slot] = *lit_streams[slot]++;
      final_len--;
    }
  }
};



template<typename Mode, bool MultiCmd>
VECTORIZED
bool Leviathan_ProcessLz(LeviathanLzTable *lzt, uint8 *dst,
                         uint8 *dst_start, uint8 *dst_end, uint8 *window_base) {
  const uint8 *cmd_stream = lzt->cmd_stream,
              *cmd_stream_end = cmd_stream + lzt->cmd_stream_size;
  const int *len_stream = lzt->len_stream;
  const int *len_stream_end = len_stream + lzt->len_stream_size;
  
  const int *offs_stream = lzt->offs_stream;
  const int *offs_stream_end = offs_stream + lzt->offs_stream_size;
  const byte *copyfrom;
  uint8 *match_zone_end = (dst_end - dst_start >= 16) ? dst_end - 16 : dst_start;

  int32 recent_offs[16];
  recent_offs[8] = recent_offs[9] = recent_offs[10] = recent_offs[11] = -8;
  recent_offs[12] = recent_offs[13] = recent_offs[14] = -8;

  size_t offset = -8;

  Mode mode(lzt, dst_start);

  uint32 cmd_stream_left;
  const uint8 *multi_cmd_stream[8], **cmd_stream_ptr;
  if (MultiCmd) {
    for (size_t i = 0; i != 8; i++)
      multi_cmd_stream[i] = lzt->multi_cmd_ptr[(i - (uintptr_t)dst_start) & 7];
    cmd_stream_left = lzt->cmd_stream_size;
    cmd_stream_ptr = &multi_cmd_stream[(uintptr_t)dst & 7];
    cmd_stream = *cmd_stream_ptr;
  }

  for(;;) {
    uint32 cmd;
    
    if (!MultiCmd) {
      if (cmd_stream >= cmd_stream_end)
        break;
      cmd = *cmd_stream++;
    } else {
      if (cmd_stream_left == 0)
        break;
      cmd_stream_left--;
      cmd = *cmd_stream;
      *cmd_stream_ptr = cmd_stream + 1;
    }

    uint32 offs_index = cmd >> 5;
    uint32 matchlen = (cmd & 7) + 2;

    recent_offs[15] = *offs_stream;

    if (!mode.CopyLiterals(cmd, dst, len_stream, match_zone_end, offset))
      return false;

    offset = recent_offs[(size_t)offs_index + 8];

    // Permute the recent offsets table
    // FIXME: if this is intended to stay within the table's bounds, it generates the wrong result on GCC 10.2.0 (array accesses are scaled to m128i)
    // MS C result is unknown, so add a preprocessor gate.
    #ifndef __GNUC__
    __m128i temp = _mm_loadu_si128((const __m128i *)&recent_offs[(size_t)offs_index + 4]);
    _mm_storeu_si128((__m128i *)&recent_offs[(size_t)offs_index + 1], _mm_loadu_si128((const __m128i *)&recent_offs[offs_index]));
    _mm_storeu_si128((__m128i *)&recent_offs[(size_t)offs_index + 5], temp);
    #else
    // Is same as above, but relies on auto-vectorization rather than intrinsics.
    MOVE32(&recent_offs[offs_index+1], &recent_offs[offs_index]);
    #endif
    
    recent_offs[8] = (int32)offset;
    offs_stream += offs_index == 7;

    if ((uintptr_t)offset < (uintptr_t)(window_base - dst))
      return false;  // offset out of bounds
    copyfrom = dst + offset;

    if (matchlen == 9) {
      if (len_stream >= len_stream_end)
        return false;  // len stream empty
      matchlen = *--len_stream_end + 6;
      COPY_64(dst, copyfrom);
      COPY_64(dst + 8, copyfrom + 8);
      uint8 *next_dst = dst + matchlen;
      if (MultiCmd)
        cmd_stream = *(cmd_stream_ptr = &multi_cmd_stream[(uintptr_t)next_dst & 7]);
      if (matchlen > 16) {
        if (matchlen > (uintptr_t)(dst_end - 8 - dst))
          return false;  // no space in buf
        COPY_64(dst + 16, copyfrom + 16);
        do {
          COPY_64(dst + 24, copyfrom + 24);
          matchlen -= 8;
          dst += 8;
          copyfrom += 8;
        } while (matchlen > 24);
      }
      dst = next_dst;
    } else {
      COPY_64(dst, copyfrom);
      dst += matchlen;
      if (MultiCmd)
        cmd_stream = *(cmd_stream_ptr = &multi_cmd_stream[(uintptr_t)dst & 7]);
    }
  }

  // check for incorrect input
  if (offs_stream != offs_stream_end || len_stream != len_stream_end)
    return false;

  // copy final literals
  if (dst < dst_end) {
    mode.CopyFinalLiterals(dst_end - dst, dst, offset);
  } else if (dst != dst_end) {
    return false;
  }
  return true;
}


bool Leviathan_ProcessLzRuns(int chunk_type, byte *dst, int dst_size, int offset, LeviathanLzTable *lzt) {
  uint8 *dst_cur = dst + (offset == 0 ? 8 : 0);
  uint8 *dst_end = dst + dst_size;
  uint8 *dst_start = dst - offset;
  
  if (lzt->cmd_stream != NULL) {
    // single cmd mode
    switch (chunk_type) {
    case 0:
      return Leviathan_ProcessLz<LeviathanModeSub, false>(lzt, dst_cur, dst, dst_end, dst_start);
    case 1:
      return Leviathan_ProcessLz<LeviathanModeRaw, false>(lzt, dst_cur, dst, dst_end, dst_start);
    case 2:
      return Leviathan_ProcessLz<LeviathanModeLamSub, false>(lzt, dst_cur, dst, dst_end, dst_start);
    case 3:
      return Leviathan_ProcessLz<LeviathanModeSubAnd3, false>(lzt, dst_cur, dst, dst_end, dst_start);
    case 4:
      return Leviathan_ProcessLz<LeviathanModeO1, false>(lzt, dst_cur, dst, dst_end, dst_start);
    case 5:
      return Leviathan_ProcessLz<LeviathanModeSubAndF, false>(lzt, dst_cur, dst, dst_end, dst_start);
    }
  } else {
    // multi cmd mode
    switch (chunk_type) {
    case 0:
      return Leviathan_ProcessLz<LeviathanModeSub, true>(lzt, dst_cur, dst, dst_end, dst_start);
    case 1:
      return Leviathan_ProcessLz<LeviathanModeRaw, true>(lzt, dst_cur, dst, dst_end, dst_start);
    case 2:
      return Leviathan_ProcessLz<LeviathanModeLamSub, true>(lzt, dst_cur, dst, dst_end, dst_start);
    case 3:
      return Leviathan_ProcessLz<LeviathanModeSubAnd3, true>(lzt, dst_cur, dst, dst_end, dst_start);
    case 4:
      return Leviathan_ProcessLz<LeviathanModeO1, true>(lzt, dst_cur, dst, dst_end, dst_start);
    case 5:
      return Leviathan_ProcessLz<LeviathanModeSubAndF, true>(lzt, dst_cur, dst, dst_end, dst_start);
    }

  }
  return false;
}

// Decode one 256kb big quantum block. It's divided into two 128k blocks
// internally that are compressed separately but with a shared history.
int Leviathan_DecodeQuantum(byte *dst, byte *dst_end, byte *dst_start,
                            const byte *src, const byte *src_end,
                            byte *scratch, byte *scratch_end) {
  const byte *src_in = src;
  int mode, chunkhdr, dst_count, src_used, written_bytes;

  while (dst_end - dst != 0) {
    dst_count = dst_end - dst;
    if (dst_count > 0x20000) dst_count = 0x20000;
    if (src_end - src < 4)
      return -1;
    chunkhdr = src[2] | src[1] << 8 | src[0] << 16;
    if (!(chunkhdr & 0x800000)) {
      // Stored as entropy without any match copying.
      byte *out = dst;
      src_used = Kraken_DecodeBytes(&out, src, src_end, &written_bytes, dst_count, false, scratch, scratch_end);
      if (src_used < 0 || written_bytes != dst_count)
        return -1;
    } else {
      src += 3;
      src_used = chunkhdr & 0x7FFFF;
      mode = (chunkhdr >> 19) & 0xF;
      if (src_end - src < src_used)
        return -1;
      if (src_used < dst_count) {
        size_t scratch_usage = Min(Min(3 * dst_count + 32 + 0xd000, 0x6C000), scratch_end - scratch);
        if (scratch_usage < sizeof(LeviathanLzTable))
          return -1;
        if (!Leviathan_ReadLzTable(mode,
            src, src + src_used,
            dst, dst_count,
            dst - dst_start,
            scratch + sizeof(LeviathanLzTable), scratch + scratch_usage,
            (LeviathanLzTable*)scratch))
          return -1;
        if (!Leviathan_ProcessLzRuns(mode, dst, dst_count, dst - dst_start, (LeviathanLzTable*)scratch))
          return -1;
      } else if (src_used > dst_count || mode != 0) {
        return -1;
      } else {
        memmove(dst, src, dst_count);
      }
    }
    src += src_used;
    dst += dst_count;
  }
  return src - src_in;
}
