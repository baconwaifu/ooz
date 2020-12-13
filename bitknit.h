#pragma once

struct BitknitState;

size_t Bitknit_Decode(const byte *src, const byte *src_end, byte *dst, byte *dst_end, byte *dst_start, BitknitState *bk);
void BitknitState_Init(BitknitState *bk);
