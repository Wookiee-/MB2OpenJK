#pragma once
#include "qcommon/q_shared.h"

// C++ optimized static huffman prototypes
void StaticHuff_PutBit( byte* fout, int bitIndex, int bit );
int  StaticHuff_PutSymbol( byte* fout, int offset, int symbol );
int  StaticHuff_GetBit( const byte* buffer, int bitIndex );
int  StaticHuff_GetSymbol( unsigned int* symbol, const byte* buffer, int bitIndex );