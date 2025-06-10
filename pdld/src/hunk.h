/******************************************************************************
 * @file            hunk.h
 *
 * Released to the public domain.
 *
 * Anyone and anything may copy, edit, publish, use, compile, sell and
 * distribute this work and all its parts in any form for any purpose,
 * commercial and non-commercial, without any restrictions, without
 * complying with any conditions and by any means.
 *****************************************************************************/
#define HUNK_UNIT 0x3E7
#define HUNK_NAME 0x3E8
#define HUNK_CODE 0x3E9
#define HUNK_DATA 0x3EA
#define HUNK_BSS 0x3EB
#define HUNK_RELOC32 0x3EC

#define HUNK_EXT 0x3EF

#define HUNK_END 0x3F2
#define HUNK_HEADER 0x3F3

#define EXT_SYMB 0
#define EXT_DEF 1
#define EXT_ABS 2
#define EXT_RES 3
#define EXT_REF32 129
#define EXT_COMMON 130
#define EXT_REF16 131
#define EXT_REF8 132

#define EXT_RELREF32 136
