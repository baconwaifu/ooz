
#include "stdafx.h"

// Header in front of each 256k block
typedef struct KrakenHeader {
  // Type of decoder used, 6 means kraken
  int decoder_type;

  // Whether to restart the decoder
  bool restart_decoder;

  // Whether this block is uncompressed
  bool uncompressed;

  // Whether this block uses checksums.
  bool use_checksums;
} KrakenHeader;

// Additional header in front of each 256k block ("quantum").
typedef struct KrakenQuantumHeader {
  // The compressed size of this quantum. If this value is 0 it means
  // the quantum is a special quantum such as memset.
  uint32 compressed_size;
  // If checksums are enabled, holds the checksum.
  uint32 checksum;
  // Two flags
  uint8 flag1;
  uint8 flag2;
  // Whether the whole block matched a previous block
  uint32 whole_match_distance;
} KrakenQuantumHeader;

// Kraken decompression happens in two phases, first one decodes
// all the literals and copy lengths using huffman and second
// phase runs the copy loop. This holds the tables needed by stage 2.
typedef struct KrakenLzTable {
  // Stream of (literal, match) pairs. The flag byte contains
  // the length of the match, the length of the literal and whether
  // to use a recent offset.
  byte *cmd_stream;
  int cmd_stream_size;

  // Holds the actual distances in case we're not using a recent
  // offset.
  int *offs_stream;
  int offs_stream_size;

  // Holds the sequence of literals. All literal copying happens from
  // here.
  byte *lit_stream;
  int lit_stream_size;

  // Holds the lengths that do not fit in the flag stream. Both literal
  // lengths and match length are stored in the same array.
  int *len_stream;
  int len_stream_size;
} KrakenLzTable;



typedef struct KrakenDecoder {
  // Updated after the |*_DecodeStep| function completes to hold
  // the number of bytes read and written.
  int src_used, dst_used;

  // Pointer to a 256k buffer that holds the intermediate state
  // in between decode phase 1 and 2.
  byte *scratch;
  size_t scratch_size;

  KrakenHeader hdr;
} KrakenDecoder;

