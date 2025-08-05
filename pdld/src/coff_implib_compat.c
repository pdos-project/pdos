/******************************************************************************
 * @file            coff_implib_compat.c
 *
 * Released to the public domain.
 *
 * Anyone and anything may copy, edit, publish, use, compile, sell and
 * distribute this work and all its parts in any form for any purpose,
 * commercial and non-commercial, without any restrictions, without
 * complying with any conditions and by any means.
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ld.h"
#include "bytearray.h"
#include "coff.h"
#include "xmalloc.h"

#include "coff_bytearray.h"

#include "coff_implib_compat.h"

static char *no_suffix_name;
static char *symbol_names[3];
static size_t thunk_size;

static unsigned short get_Machine (void)
{
    switch (ld_state->target_machine) {
        case LD_TARGET_MACHINE_I386: return IMAGE_FILE_MACHINE_I386;
        case LD_TARGET_MACHINE_X64: return IMAGE_FILE_MACHINE_AMD64;
        
        case LD_TARGET_MACHINE_ARM: return IMAGE_FILE_MACHINE_ARMNT;
        
        case LD_TARGET_MACHINE_AARCH64: return IMAGE_FILE_MACHINE_ARM64;

        default: return IMAGE_FILE_MACHINE_UNKNOWN;
    }
}

static void write_archive_member_header (unsigned char *pos, const char *Name, unsigned long Size, unsigned long lu_timestamp)
{
    struct IMAGE_ARCHIVE_MEMBER_HEADER_internal member_hdr;

    memset (&member_hdr, ' ', sizeof (member_hdr));
    
    member_hdr.Name[sprintf (member_hdr.Name, Name)] = ' ';
    member_hdr.Date[sprintf (member_hdr.Date, "%lu", lu_timestamp)] = ' ';
    member_hdr.UserID[0] = '0';
    member_hdr.GroupID[0] = '0';
    member_hdr.Mode[0] = '0';
    member_hdr.Size[sprintf (member_hdr.Size, "%lu", Size)] = ' ';
    memcpy (member_hdr.EndOfHeader, IMAGE_ARCHIVE_MEMBER_HEADER_END_OF_HEADER, sizeof (member_hdr.EndOfHeader));
    
    write_struct_IMAGE_ARCHIVE_MEMBER_HEADER (pos, &member_hdr);
}

static unsigned short get_reloc_type (void)
{
    switch (ld_state->target_machine) {
        case LD_TARGET_MACHINE_I386: return IMAGE_REL_I386_DIR32NB;
        case LD_TARGET_MACHINE_X64: return IMAGE_REL_AMD64_ADDR32NB;
        
        case LD_TARGET_MACHINE_ARM: return IMAGE_REL_ARM_ADDR32NB;
        
        case LD_TARGET_MACHINE_AARCH64: return IMAGE_REL_ARM64_ADDR32NB;

        default: return 0;
    }
}

static size_t get_size_import_descriptor (void)
{
    return (SIZEOF_struct_coff_header_file
            + 2 * SIZEOF_struct_section_table_entry_file
            + SIZEOF_struct_IMPORT_Directory_Table_file
            + strlen (ld_state->output_filename) + 1
            + 3 * SIZEOF_struct_relocation_entry_file
            + 6 * SIZEOF_struct_symbol_table_entry_file
            + SIZEOF_struct_string_table_header_file
            + strlen (symbol_names[0]) + 1
            + strlen (symbol_names[1]) + 1
            + strlen (symbol_names[2]) + 1);
}

static size_t get_size_null_import_descriptor (void)
{
    return (SIZEOF_struct_coff_header_file
            + SIZEOF_struct_section_table_entry_file
            + SIZEOF_struct_IMPORT_Directory_Table_file
            + SIZEOF_struct_symbol_table_entry_file
            + SIZEOF_struct_string_table_header_file
            + sizeof ("__NULL_IMPORT_DESCRIPTOR"));
}

static size_t get_size_null_thunk_data (void)
{
    return (SIZEOF_struct_coff_header_file
            + 2 * SIZEOF_struct_section_table_entry_file
            + 2 * thunk_size
            + SIZEOF_struct_symbol_table_entry_file
            + SIZEOF_struct_string_table_header_file
            + strlen (symbol_names[2]) + 1);
}

void coff_implib_compat_calculate (size_t *file_size_p,
                                   unsigned long *num_linker_member_offsets_p,
                                   unsigned long *linker_member_size_p)
{
    size_t i;
    size_t file_size = *file_size_p;
    unsigned long num_linker_member_offsets = *num_linker_member_offsets_p;
    unsigned long linker_member_size = *linker_member_size_p;
    const char *p = strrchr (ld_state->output_filename, '.');

    if (p) {
        no_suffix_name = xstrndup (ld_state->output_filename, p - ld_state->output_filename);
    } else no_suffix_name = xstrdup (ld_state->output_filename);

    symbol_names[0] = xmalloc (sizeof ("__IMPORT_DESCRIPTOR_") + strlen (no_suffix_name));
    sprintf (symbol_names[0], "__IMPORT_DESCRIPTOR_%s", no_suffix_name);
    symbol_names[1] = xstrdup ("__NULL_IMPORT_DESCRIPTOR");
    symbol_names[2] = xmalloc (sizeof ("\x7f_NULL_THUNK_DATA") + strlen (no_suffix_name));
    sprintf (symbol_names[2], "\x7f%s_NULL_THUNK_DATA", no_suffix_name);

    if (ld_state->target_machine == LD_TARGET_MACHINE_X64
        || ld_state->target_machine == LD_TARGET_MACHINE_AARCH64) {
        thunk_size = 8;
    } else {
        thunk_size = 4;
    }

    file_size += SIZEOF_struct_IMAGE_ARCHIVE_MEMBER_HEADER_file;
    file_size += strlen (ld_state->output_filename) + 1;
    file_size = ALIGN (file_size, 2);

    for (i = 0; i < 3; i++) {
        switch (i) {
            case 0: file_size += get_size_import_descriptor (); break;
            case 1: file_size += get_size_null_import_descriptor (); break;
            case 2: file_size += get_size_null_thunk_data (); break;
        }

        file_size += SIZEOF_struct_IMAGE_ARCHIVE_MEMBER_HEADER_file;
        linker_member_size += 4 + strlen (symbol_names[i]) + 1;
        num_linker_member_offsets++;

        file_size = ALIGN (file_size, 2);
    }

    *file_size_p = file_size;
    *num_linker_member_offsets_p = num_linker_member_offsets;
    *linker_member_size_p = linker_member_size;
}

static void write_import_descriptor (unsigned char *pos)
{
    struct coff_header_internal coff_hdr = {0};
    struct section_table_entry_internal sec_hdr;
    struct symbol_table_entry_internal sym;
    struct string_table_header_internal strtab_hdr;
    struct relocation_entry_internal reloc;

    coff_hdr.Machine = get_Machine ();
    coff_hdr.NumberOfSections = 2;
    coff_hdr.PointerToSymbolTable = (SIZEOF_struct_coff_header_file
                                     + 2 * SIZEOF_struct_section_table_entry_file
                                     + SIZEOF_struct_IMPORT_Directory_Table_file
                                     + strlen (ld_state->output_filename) + 1
                                     + 3 * SIZEOF_struct_relocation_entry_file);
    coff_hdr.NumberOfSymbols = 6;
    
    write_struct_coff_header (pos, &coff_hdr);
    pos += SIZEOF_struct_coff_header_file;

    memset (&sec_hdr, 0, sizeof sec_hdr);
    memcpy (sec_hdr.Name, ".idata$2", sizeof sec_hdr.Name);
    sec_hdr.SizeOfRawData = SIZEOF_struct_IMPORT_Directory_Table_file;
    sec_hdr.PointerToRawData = SIZEOF_struct_coff_header_file + 2 * SIZEOF_struct_section_table_entry_file;
    sec_hdr.PointerToRelocations = (SIZEOF_struct_coff_header_file
                                    + 2 * SIZEOF_struct_section_table_entry_file
                                    + SIZEOF_struct_IMPORT_Directory_Table_file
                                    + strlen (ld_state->output_filename) + 1);
    sec_hdr.NumberOfRelocations = 3;
    sec_hdr.Characteristics = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_ALIGN_4BYTES | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE;
    write_struct_section_table_entry (pos, &sec_hdr);
    pos += SIZEOF_struct_section_table_entry_file;
    
    memset (&sec_hdr, 0, sizeof sec_hdr);
    memcpy (sec_hdr.Name, ".idata$6", sizeof sec_hdr.Name);
    sec_hdr.SizeOfRawData = strlen (ld_state->output_filename) + 1;
    sec_hdr.PointerToRawData = SIZEOF_struct_coff_header_file + 2 * SIZEOF_struct_section_table_entry_file + SIZEOF_struct_IMPORT_Directory_Table_file;
    sec_hdr.Characteristics = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_ALIGN_2BYTES | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE;
    write_struct_section_table_entry (pos, &sec_hdr);
    pos += SIZEOF_struct_section_table_entry_file;
    
    pos += SIZEOF_struct_IMPORT_Directory_Table_file;
    
    strcpy ((char *)pos, ld_state->output_filename);
    pos += strlen (ld_state->output_filename) + 1;

    reloc.Type = get_reloc_type ();
    reloc.VirtualAddress = offsetof (struct IMPORT_Directory_Table_file, ImportNameTableRVA);
    reloc.SymbolTableIndex = 2;
    write_struct_relocation_entry (pos, &reloc);
    pos += SIZEOF_struct_relocation_entry_file;

    reloc.VirtualAddress = offsetof (struct IMPORT_Directory_Table_file, NameRVA);
    reloc.SymbolTableIndex = 1;
    write_struct_relocation_entry (pos, &reloc);
    pos += SIZEOF_struct_relocation_entry_file;

    reloc.VirtualAddress = offsetof (struct IMPORT_Directory_Table_file, ImportAddressTableRVA);
    reloc.SymbolTableIndex = 3;
    write_struct_relocation_entry (pos, &reloc);
    pos += SIZEOF_struct_relocation_entry_file;

    memset (&sym, 0, sizeof sym);
    bytearray_write_4_bytes ((unsigned char *)(sym.Name + 4), SIZEOF_struct_string_table_header_file, LITTLE_ENDIAN);
    sym.SectionNumber = 1;
    sym.StorageClass = IMAGE_SYM_CLASS_EXTERNAL;
    write_struct_symbol_table_entry (pos, &sym);
    pos += SIZEOF_struct_symbol_table_entry_file;
    
    memset (&sym, 0, sizeof sym);
    memcpy (sym.Name, ".idata$6", sizeof sym.Name);
    sym.SectionNumber = 2;
    sym.StorageClass = IMAGE_SYM_CLASS_STATIC;
    write_struct_symbol_table_entry (pos, &sym);
    pos += SIZEOF_struct_symbol_table_entry_file;
    
    memset (&sym, 0, sizeof sym);
    memcpy (sym.Name, ".idata$4", sizeof sym.Name);
    sym.Value = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE;
    sym.SectionNumber = 0;
    sym.StorageClass = IMAGE_SYM_CLASS_SECTION;
    write_struct_symbol_table_entry (pos, &sym);
    pos += SIZEOF_struct_symbol_table_entry_file;
    
    memset (&sym, 0, sizeof sym);
    memcpy (sym.Name, ".idata$5", sizeof sym.Name);
    sym.Value = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE;
    sym.SectionNumber = 0;
    sym.StorageClass = IMAGE_SYM_CLASS_SECTION;
    write_struct_symbol_table_entry (pos, &sym);
    pos += SIZEOF_struct_symbol_table_entry_file;

    memset (&sym, 0, sizeof sym);
    bytearray_write_4_bytes ((unsigned char *)(sym.Name + 4), SIZEOF_struct_string_table_header_file + strlen (symbol_names[0]) + 1, LITTLE_ENDIAN);
    sym.SectionNumber = 0;
    sym.StorageClass = IMAGE_SYM_CLASS_EXTERNAL;
    write_struct_symbol_table_entry (pos, &sym);
    pos += SIZEOF_struct_symbol_table_entry_file;

    memset (&sym, 0, sizeof sym);
    bytearray_write_4_bytes ((unsigned char *)(sym.Name + 4), SIZEOF_struct_string_table_header_file + strlen (symbol_names[0]) + 1 + strlen (symbol_names[1]) + 1, LITTLE_ENDIAN);
    sym.SectionNumber = 0;
    sym.StorageClass = IMAGE_SYM_CLASS_EXTERNAL;
    write_struct_symbol_table_entry (pos, &sym);
    pos += SIZEOF_struct_symbol_table_entry_file;

    strtab_hdr.StringTableSize = (SIZEOF_struct_string_table_header_file
                                  + strlen (symbol_names[0]) + 1
                                  + strlen (symbol_names[1]) + 1
                                  + strlen (symbol_names[2]) + 1);
    write_struct_string_table_header (pos, &strtab_hdr);
    pos += SIZEOF_struct_string_table_header_file;

    strcpy ((char *)pos, symbol_names[0]);
    pos += strlen (symbol_names[0]) + 1;
    strcpy ((char *)pos, symbol_names[1]);
    pos += strlen (symbol_names[1]) + 1;
    strcpy ((char *)pos, symbol_names[2]);
}

static void write_null_import_descriptor (unsigned char *pos)
{
    struct coff_header_internal coff_hdr = {0};
    struct section_table_entry_internal sec_hdr;
    struct symbol_table_entry_internal sym;
    struct string_table_header_internal strtab_hdr;

    coff_hdr.Machine = get_Machine ();
    coff_hdr.NumberOfSections = 1;
    coff_hdr.PointerToSymbolTable = SIZEOF_struct_coff_header_file + SIZEOF_struct_section_table_entry_file + SIZEOF_struct_IMPORT_Directory_Table_file;
    coff_hdr.NumberOfSymbols = 1;
    
    write_struct_coff_header (pos, &coff_hdr);
    pos += SIZEOF_struct_coff_header_file;

    memset (&sec_hdr, 0, sizeof sec_hdr);
    memcpy (sec_hdr.Name, ".idata$3", sizeof sec_hdr.Name);
    sec_hdr.SizeOfRawData = SIZEOF_struct_IMPORT_Directory_Table_file;
    sec_hdr.PointerToRawData = SIZEOF_struct_coff_header_file + SIZEOF_struct_section_table_entry_file;
    sec_hdr.Characteristics = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_ALIGN_4BYTES | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE;
    write_struct_section_table_entry (pos, &sec_hdr);
    pos += SIZEOF_struct_section_table_entry_file;
    pos += SIZEOF_struct_IMPORT_Directory_Table_file;

    memset (&sym, 0, sizeof sym);
    bytearray_write_4_bytes ((unsigned char *)(sym.Name + 4), SIZEOF_struct_string_table_header_file, LITTLE_ENDIAN);
    sym.SectionNumber = 1;
    sym.StorageClass = IMAGE_SYM_CLASS_EXTERNAL;
    write_struct_symbol_table_entry (pos, &sym);
    pos += SIZEOF_struct_symbol_table_entry_file;

    strtab_hdr.StringTableSize = SIZEOF_struct_string_table_header_file + sizeof ("__NULL_IMPORT_DESCRIPTOR");
    write_struct_string_table_header (pos, &strtab_hdr);
    pos += SIZEOF_struct_string_table_header_file;

    strcpy ((char *)pos, "__NULL_IMPORT_DESCRIPTOR");
}

static void write_null_thunk_data (unsigned char *pos)
{
    struct coff_header_internal coff_hdr = {0};
    struct section_table_entry_internal sec_hdr = {""};
    struct symbol_table_entry_internal sym;
    struct string_table_header_internal strtab_hdr;

    coff_hdr.Machine = get_Machine ();
    coff_hdr.NumberOfSections = 2;
    coff_hdr.PointerToSymbolTable = SIZEOF_struct_coff_header_file + 2 * SIZEOF_struct_section_table_entry_file + 2 * thunk_size;
    coff_hdr.NumberOfSymbols = 1;
    
    write_struct_coff_header (pos, &coff_hdr);
    pos += SIZEOF_struct_coff_header_file;

    memset (&sec_hdr, 0, sizeof sec_hdr);
    memcpy (sec_hdr.Name, ".idata$5", sizeof sec_hdr.Name);
    sec_hdr.SizeOfRawData = thunk_size;
    sec_hdr.PointerToRawData = SIZEOF_struct_coff_header_file + 2 * SIZEOF_struct_section_table_entry_file;
    sec_hdr.Characteristics = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_ALIGN_4BYTES | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE;
    write_struct_section_table_entry (pos, &sec_hdr);
    pos += SIZEOF_struct_section_table_entry_file;
    
    memset (&sec_hdr, 0, sizeof sec_hdr);
    memcpy (sec_hdr.Name, ".idata$4", sizeof sec_hdr.Name);
    sec_hdr.SizeOfRawData = thunk_size;
    sec_hdr.PointerToRawData = SIZEOF_struct_coff_header_file + 2 * SIZEOF_struct_section_table_entry_file + thunk_size;
    sec_hdr.Characteristics = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_ALIGN_4BYTES | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE;
    write_struct_section_table_entry (pos, &sec_hdr);
    pos += SIZEOF_struct_section_table_entry_file;
    pos += 2 * thunk_size;

    memset (&sym, 0, sizeof sym);
    bytearray_write_4_bytes ((unsigned char *)(sym.Name + 4), 4, LITTLE_ENDIAN);
    sym.SectionNumber = 1;
    sym.StorageClass = IMAGE_SYM_CLASS_EXTERNAL;
    write_struct_symbol_table_entry (pos, &sym);
    pos += SIZEOF_struct_symbol_table_entry_file;

    strtab_hdr.StringTableSize = SIZEOF_struct_string_table_header_file + strlen (symbol_names[2]) + 1;
    write_struct_string_table_header (pos, &strtab_hdr);
    pos += SIZEOF_struct_string_table_header_file;

    strcpy ((char *)pos, symbol_names[2]);
}

void coff_implib_compat_write (unsigned char *file,
                               unsigned char **pos_p,
                               unsigned char **offset_pos_p,
                               unsigned char **string_table_pos_p,
                               unsigned long lu_timestamp)
{
    size_t i;
    unsigned char *pos = *pos_p;
    unsigned char *offset_pos = *offset_pos_p;
    unsigned char *string_table_pos = *string_table_pos_p;

    /* All archive members must be named after the DLL for compatibility
     * because other linkers pointlessly depend on it.
     */
    write_archive_member_header (pos, IMAGE_ARCHIVE_LONGNAMES_MEMBER_Name,
                                 strlen (ld_state->output_filename) + 1,
                                 lu_timestamp);
    pos += SIZEOF_struct_IMAGE_ARCHIVE_MEMBER_HEADER_file;
    strcpy ((char *)pos, ld_state->output_filename);
    pos += strlen (ld_state->output_filename) + 1;
    pos = file + ALIGN (pos - file, 2);
    
    for (i = 0; i < 3; i++)
    {
        bytearray_write_4_bytes (offset_pos, pos - file, BIG_ENDIAN);
        offset_pos += 4;
        
        strcpy ((char *)string_table_pos, symbol_names[i]);
        string_table_pos += strlen (symbol_names[i]) + 1;
    
        switch (i) {
            case 0:
                write_archive_member_header (pos, "/0",
                                             get_size_import_descriptor (),
                                             lu_timestamp);
                break;
            
            case 1:
                write_archive_member_header (pos, "/0",
                                             get_size_null_import_descriptor (),
                                             lu_timestamp);
                break;

            case 2:
                write_archive_member_header (pos, "/0",
                                             get_size_null_thunk_data (),
                                             lu_timestamp);
                break;
        }
        
        pos += SIZEOF_struct_IMAGE_ARCHIVE_MEMBER_HEADER_file;
        
        switch (i) {
            case 0:
                write_import_descriptor (pos);
                pos += get_size_import_descriptor ();
                break;
            
            case 1:
                write_null_import_descriptor (pos);
                pos += get_size_null_import_descriptor ();
                break;
            
            case 2:
                write_null_thunk_data (pos);
                pos += get_size_null_thunk_data ();
                break;
        }
        
        pos = file + ALIGN (pos - file, 2);
    }

    free (symbol_names[0]);
    free (symbol_names[1]);
    free (symbol_names[2]);
    free (no_suffix_name);

    *pos_p = pos;
    *offset_pos_p = offset_pos;
    *string_table_pos_p = string_table_pos;
}

