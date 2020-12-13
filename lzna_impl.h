#pragma once

#include "stdafx.h"
#include "vectorize.h"
#include "kraken_impl.h"

struct LznaState;

int LZNA_DecodeQuantum(byte *dst, byte *dst_end, byte *dst_start,
                       const byte *src, const byte *src_end,
                       struct LznaState *lut);
void LZNA_InitLookup(LznaState *lut);
const byte *LZNA_ParseWholeMatchInfo(const byte *p, uint32 *dist);
const byte *LZNA_ParseQuantumHeader(KrakenQuantumHeader *hdr, const byte *p, bool use_checksum, int raw_len);
