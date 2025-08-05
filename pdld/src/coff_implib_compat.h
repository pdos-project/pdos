/******************************************************************************
 * @file            coff_implib_compat.h
 *
 * Released to the public domain.
 *
 * Anyone and anything may copy, edit, publish, use, compile, sell and
 * distribute this work and all its parts in any form for any purpose,
 * commercial and non-commercial, without any restrictions, without
 * complying with any conditions and by any means.
 *****************************************************************************/

#define coff_implib_compat_write cimpcw

void coff_implib_compat_calculate (size_t *file_size_p,
                                   unsigned long *num_linker_member_offsets_p,
                                   unsigned long *linker_member_size_p);
void coff_implib_compat_write (unsigned char *file,
                               unsigned char **pos_p,
                               unsigned char **offset_pos_p,
                               unsigned char **string_table_pos_p,
                               unsigned long lu_timestamp);
