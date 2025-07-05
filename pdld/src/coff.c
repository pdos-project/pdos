/******************************************************************************
 * @file            coff.c
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
#include <time.h>

#include "ld.h"
#include "bytearray.h"
#include "coff.h"
#include "xmalloc.h"

#include "coff_bytearray.h"

#include "coff_implib_compat.h"

#ifdef CONV_CHARSET
void ftgtchsmem (void *mem, size_t size)
{
    char *p = mem;
    
    while (size--) {
        *p = ftgtchs(*p);
        p++;
    }
}

void ttgtchsmem (void *mem, size_t size)
{
    char *p = mem;
    
    while (size--) {
        *p = ttgtchs(*p);
        p++;
    }
}
#else
#define ftgtchsmem(mem, size)
#define ttgtchsmem(mem, size)
#endif

/* Specification does not seem to say that there must actually be 16 IMAGE_DATA_DIRECTORYs
 * but certain PE/COFF tool does not accept the file if they are not there
 * and it is easier to generate them all even if they are unused. */
#define NUMBER_OF_DATA_DIRECTORIES 16

/* "__imp_" added in front of the "_" prefix means
 * that the symbol is imported directly from Import Address Table
 * instead of from stub generated in .text. */
#define IMP_PREFIX_STR "__imp_"
#define IMP_PREFIX_LEN 6

static int insert_timestamp = 1;
static int export_all_symbols = 0;
static int kill_at = 0;
static int generate_reloc_section = 1;
static int can_be_relocated = 0;
static int nx_compat = 1;

/* Some other linkers require extra archive members in import libraries. */
static int implib_compat = 0;

static int leading_underscore = 0;
static int arm_thumb_mode = -1;

static unsigned long SectionAlignment = DEFAULT_SECTION_ALIGNMENT;
static unsigned long FileAlignment = DEFAULT_FILE_ALIGNMENT;
static unsigned short MajorSubsystemVersion = 4;
static unsigned short MinorSubsystemVersion = 0;
static unsigned short Subsystem = IMAGE_SUBSYSTEM_WINDOWS_CUI;
static unsigned long SizeOfStackReserve = 0x200000;
static unsigned long SizeOfStackCommit = 0x1000;

static unsigned char *stub_file;
static size_t stub_size;
static long size_of_headers;

static address_type size_of_code = 0;
static address_type size_of_initialized_data = 0;
static address_type size_of_uninitialized_data = 0;
static address_type base_of_code = 0;
static address_type base_of_data = 0;

static struct section_part *iat_first_part = NULL;
static struct section_part *iat_last_part = NULL;

static struct section *last_section;

static char *current_import_dll_name = NULL;

struct name_list {
    struct name_list *next;
    int info;
    char *name;
};

enum export_type {
    EXPORT_TYPE_CODE,
    EXPORT_TYPE_DATA,
    EXPORT_TYPE_CONST
};

struct export_name {
    char *name;
    char *name_no_at;
    enum export_type export_type;
};

struct exclude_symbol {
    struct exclude_symbol *next;
    char *name;
};

static struct name_list *export_name_list = NULL;
static struct name_list **last_export_name_list_p = &export_name_list;
static struct exclude_symbol *exclude_symbols = NULL;

/* Shares the stub with LX. */
void coff_get_stub_file (unsigned char **stub_file_p, size_t *stub_size_p)
{
    *stub_file_p = stub_file;
    *stub_size_p = stub_size;
}

unsigned long coff_get_SizeOfStackCommit (void)
{
    return SizeOfStackCommit;
}

static int check_Machine (unsigned short Machine, const char *filename)
{
    switch (Machine) {
        /* Some machines do not have their own Machine value yet
         * and use IMAGE_FILE_MACHINE_UNKNOWN instead.
         */
        case IMAGE_FILE_MACHINE_UNKNOWN: return 0;
        
        case IMAGE_FILE_MACHINE_AMD64:
            if (ld_state->target_machine == LD_TARGET_MACHINE_X64
                || ld_state->target_machine == LD_TARGET_MACHINE_UNKNOWN) {
                ld_state->target_machine = LD_TARGET_MACHINE_X64;
            } else {
                ld_error ("%s: Machine field mismatch between objects", filename);
                return 1;
            }
            break;
        
        case IMAGE_FILE_MACHINE_I386:
            if (ld_state->target_machine == LD_TARGET_MACHINE_I386
                || ld_state->target_machine == LD_TARGET_MACHINE_UNKNOWN) {
                ld_state->target_machine = LD_TARGET_MACHINE_I386;
            } else {
                ld_error ("%s: Machine field mismatch between objects", filename);
                return 1;
            }
            leading_underscore = 1;
            break;

        case IMAGE_FILE_MACHINE_ARM:
        case IMAGE_FILE_MACHINE_ARMNT:
        case IMAGE_FILE_MACHINE_THUMB:
            if (ld_state->target_machine == LD_TARGET_MACHINE_ARM
                || ld_state->target_machine == LD_TARGET_MACHINE_UNKNOWN) {
                ld_state->target_machine = LD_TARGET_MACHINE_ARM;
            } else {
                ld_error ("%s: Machine field mismatch between objects", filename);
                return 1;
            }
            break;

        case IMAGE_FILE_MACHINE_ARM64:
            if (ld_state->target_machine == LD_TARGET_MACHINE_AARCH64
                || ld_state->target_machine == LD_TARGET_MACHINE_UNKNOWN) {
                ld_state->target_machine = LD_TARGET_MACHINE_AARCH64;
            } else {
                ld_error ("%s: Machine field mismatch between objects", filename);
                return 1;
            }
            break;

        case IMAGE_FILE_MACHINE_M68K:
            if (ld_state->target_machine == LD_TARGET_MACHINE_M68K
                || ld_state->target_machine == LD_TARGET_MACHINE_UNKNOWN) {
                ld_state->target_machine = LD_TARGET_MACHINE_M68K;
            } else {
                ld_error ("%s: Machine field mismatch between objects", filename);
                return 1;
            }
            leading_underscore = 1;
            break;

        default:
            ld_error ("%s: unrecognized Machine %#x", filename, Machine);
            return 1;
    }

    return 0;
}

static unsigned short get_Machine (void)
{
    switch (ld_state->target_machine) {
        case LD_TARGET_MACHINE_I386: return IMAGE_FILE_MACHINE_I386;
        case LD_TARGET_MACHINE_X64: return IMAGE_FILE_MACHINE_AMD64;
        
        case LD_TARGET_MACHINE_ARM:
            /* EFI rejects Thumb-2 little endian and ARM little endian,
             * Windows rejects Thumb,
             * so appropriate Machine is picked regardless of input Machine.
             */
            if (Subsystem == IMAGE_SUBSYSTEM_EFI_APPLICATION
                || Subsystem == IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER
                || Subsystem == IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER
                || Subsystem == IMAGE_SUBSYSTEM_EFI_ROM) {
                return IMAGE_FILE_MACHINE_THUMB;
            } else {
                return IMAGE_FILE_MACHINE_ARMNT;
            }
        
        case LD_TARGET_MACHINE_AARCH64: return IMAGE_FILE_MACHINE_ARM64;
        case LD_TARGET_MACHINE_M68K: return IMAGE_FILE_MACHINE_M68K;

        default: return IMAGE_FILE_MACHINE_UNKNOWN;
    }
}

static unsigned long translate_section_flags_to_Characteristics (flag_int flags)
{
    unsigned long Characteristics = 0;

    if (!(flags & SECTION_FLAG_READONLY)) {
        Characteristics |= IMAGE_SCN_MEM_WRITE;
    }

    if (flags & SECTION_FLAG_CODE) {
        Characteristics |= IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE;
    }

    if (flags & SECTION_FLAG_DATA) {
        Characteristics |= IMAGE_SCN_CNT_INITIALIZED_DATA;
    }

    if (flags & SECTION_FLAG_NEVER_LOAD) {
        Characteristics |= IMAGE_SCN_TYPE_NOLOAD;
    }

    if (flags & SECTION_FLAG_DEBUGGING) {
        Characteristics |= IMAGE_SCN_LNK_INFO;
    }

    if (flags & SECTION_FLAG_EXCLUDE) {
        Characteristics |= IMAGE_SCN_LNK_REMOVE;
    }

    if (!(flags & SECTION_FLAG_NOREAD)) {
        Characteristics |= IMAGE_SCN_MEM_READ;
    }

    if (flags & SECTION_FLAG_SHARED) {
        Characteristics |= IMAGE_SCN_MEM_SHARED;
    }

    /* .bss */
    if ((flags & SECTION_FLAG_ALLOC) && !(flags & SECTION_FLAG_LOAD)) {
        Characteristics |= IMAGE_SCN_CNT_UNINITIALIZED_DATA;
    }

    return Characteristics;
}

static flag_int translate_Characteristics_to_section_flags (unsigned long Characteristics)
{
    flag_int flags = 0;

    if (!(Characteristics & IMAGE_SCN_MEM_WRITE)) flags |= SECTION_FLAG_READONLY;

    if (Characteristics & (IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE)) flags |= SECTION_FLAG_CODE;

    if (Characteristics & IMAGE_SCN_CNT_INITIALIZED_DATA) flags |= SECTION_FLAG_DATA;

    if (Characteristics & IMAGE_SCN_TYPE_NOLOAD) flags |= SECTION_FLAG_NEVER_LOAD;
    
    if (Characteristics & IMAGE_SCN_LNK_INFO) flags |= SECTION_FLAG_DEBUGGING;
    
    if (Characteristics & IMAGE_SCN_LNK_REMOVE) flags |= SECTION_FLAG_EXCLUDE;

    if (!(Characteristics & IMAGE_SCN_MEM_READ)) flags |= SECTION_FLAG_NOREAD;

    if (Characteristics & IMAGE_SCN_MEM_SHARED) flags |= SECTION_FLAG_SHARED;

    if (Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA) flags |= SECTION_FLAG_ALLOC;

    return flags;
}

static address_type translate_Characteristics_to_alignment (unsigned long Characteristics)
{
    unsigned long alignment = 1;

    Characteristics &= IMAGE_SCN_ALIGN_8192BYTES | IMAGE_SCN_ALIGN_1BYTES;
    
    if (Characteristics == IMAGE_SCN_ALIGN_1BYTES) alignment = 1;
    if (Characteristics == IMAGE_SCN_ALIGN_2BYTES) alignment = 2;
    if (Characteristics == IMAGE_SCN_ALIGN_4BYTES) alignment = 4;
    if (Characteristics == IMAGE_SCN_ALIGN_8BYTES) alignment = 8;
    if (Characteristics == IMAGE_SCN_ALIGN_16BYTES) alignment = 16;
    if (Characteristics == IMAGE_SCN_ALIGN_32BYTES) alignment = 32;
    if (Characteristics == IMAGE_SCN_ALIGN_64BYTES) alignment = 64;
    if (Characteristics == IMAGE_SCN_ALIGN_128BYTES) alignment = 128;
    if (Characteristics == IMAGE_SCN_ALIGN_256BYTES) alignment = 256;
    if (Characteristics == IMAGE_SCN_ALIGN_512BYTES) alignment = 512;
    if (Characteristics == IMAGE_SCN_ALIGN_1024BYTES) alignment = 1024;
    if (Characteristics == IMAGE_SCN_ALIGN_2048BYTES) alignment = 2048;
    if (Characteristics == IMAGE_SCN_ALIGN_4096BYTES) alignment = 4096;
    if (Characteristics == IMAGE_SCN_ALIGN_8192BYTES) alignment = 8192;

    return alignment;
}

static void write_sections (unsigned char *file)
{
    unsigned char *pos;
    struct section *section;

    pos = file + size_of_headers;

    for (section = all_sections; section; section = section->next) {
        
        struct section_table_entry_internal *hdr = xmalloc (sizeof (*hdr));

        section->object_dependent_data = hdr;

        /* Names should be just truncated as no string table exists. */
        memset (hdr->Name, '\0', sizeof (hdr->Name));
        memcpy (hdr->Name,
                section->name,
                (strlen (section->name) >= sizeof (hdr->Name))
                ? sizeof (hdr->Name)
                : strlen (section->name));
        ttgtchsmem(hdr->Name, sizeof (hdr->Name));

        hdr->VirtualSize = section->total_size;
        hdr->VirtualAddress = section->rva;

        if (!section->is_bss) {
            hdr->SizeOfRawData = ALIGN (section->total_size, FileAlignment);
            hdr->PointerToRawData = pos - file;

            section_write (section, pos);
            pos += ALIGN (section->total_size, FileAlignment);
        } else {
            hdr->SizeOfRawData = 0;
            hdr->PointerToRawData = 0;
        }
        hdr->PointerToRelocations = 0;
        hdr->PointerToLinenumbers = 0;

        hdr->NumberOfRelocations = 0;
        hdr->NumberOfLinenumbers = 0;

        hdr->Characteristics = translate_section_flags_to_Characteristics (section->flags);

        if (hdr->Characteristics & IMAGE_SCN_CNT_CODE) {
            if (!base_of_code) base_of_code = hdr->VirtualAddress;
            size_of_code += hdr->VirtualSize;
        } else if (hdr->Characteristics & IMAGE_SCN_CNT_INITIALIZED_DATA) {
            if (!base_of_data) base_of_data = hdr->VirtualAddress;
            size_of_initialized_data += hdr->VirtualSize;
        } else if (hdr->Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA) {
            size_of_uninitialized_data += hdr->VirtualSize;
        }

        last_section = section;
    }
}

address_type coff_get_base_address (void)
{    
    if (ld_state->target_machine == LD_TARGET_MACHINE_AARCH64) return DEFAULT_ARM64_IMAGE_BASE;
    if (ld_state->create_shared_library) return DEFAULT_DLL_IMAGE_BASE;
    return DEFAULT_EXE_IMAGE_BASE;
}

address_type coff_get_first_section_rva (void)
{
    return ALIGN (size_of_headers, SectionAlignment);
}

static void export_symbol_callback (struct symbol *symbol)
{
    if (symbol_is_undefined (symbol)) return;
    if (symbol->flags & SYMBOL_FLAG_EXCLUDE_EXPORT) return;

    {
        struct name_list *name_list;
        name_list = xmalloc (sizeof (*name_list));
        if (leading_underscore && symbol->name[0] == '_') {
            name_list->name = xstrdup (symbol->name + 1);
        } else {
            name_list->name = xstrdup (symbol->name);
        }
        /* Not sure if this guessing is the correct way
         * of determing whether symbol should be exported as data. */
        name_list->info = (symbol->part->section->flags & SECTION_FLAG_CODE) ? 0 : 1;
        name_list->next = NULL;
        *last_export_name_list_p = name_list;
        last_export_name_list_p = &name_list->next;
    }
}

static void export_all_generate_names (void)
{
    struct name_list *name_list, *next_name_list;
    struct exclude_symbol *exclude_symbol;

    for (name_list = export_name_list;
         name_list;
         name_list = next_name_list) {
        next_name_list = name_list->next;
        free (name_list->name);
        free (name_list);
    }

    export_name_list = NULL;
    last_export_name_list_p = &export_name_list;

    for (exclude_symbol = exclude_symbols;
         exclude_symbol;
         exclude_symbol = exclude_symbol->next) {
        struct symbol *symbol;

        if ((symbol = symbol_find (exclude_symbol->name))) {
            symbol->flags |= SYMBOL_FLAG_EXCLUDE_EXPORT;
        }
    }
    
    symbols_for_each_global (&export_symbol_callback);
}

static void exclude_symbols_free (void)
{
    struct exclude_symbol *exclude_symbol;

    for (exclude_symbol = exclude_symbols;
         exclude_symbol;
         exclude_symbol = exclude_symbols) {
        exclude_symbols = exclude_symbol->next;
        free (exclude_symbol->name);
        free (exclude_symbol);
    }
}

static char *unprefix_name (const char *orig_name)
{
    char *name;
    
    if (orig_name[0] == '_') {
        name = xmalloc (strlen (orig_name));
        strcpy (name, orig_name + 1);
    } else {
        name = xstrdup (orig_name);
    }

    return name;
}

static char *unat_name (const char *orig_name)
{
    char *name;
    const char *at_p;
    
    at_p = strchr (orig_name, '@');
    if (at_p) {
        name = xmalloc (at_p - orig_name + 1);
        memcpy (name, orig_name, at_p - orig_name);
        name[at_p - orig_name] = '\0';
    } else {
        name = xstrdup (orig_name);
    }

    return name;
}

static char *undecorate_name (const char *orig_name)
{
    char *name;
    const char *at_p;

    if (orig_name[0] == '_') orig_name++;
    
    at_p = strchr (orig_name, '@');
    if (at_p) {
        name = xmalloc (at_p - orig_name + 1);
        memcpy (name, orig_name, at_p - orig_name);
        name[at_p - orig_name] = '\0';
    } else {
        name = xstrdup (orig_name);
    }

    return name;
}

static int export_name_compar (const void *a, const void *b)
{
    return strcmp (((struct export_name *)a)->name, ((struct export_name *)b)->name);
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

static void write_implib (const struct export_name *export_names,
                          size_t num_names,
                          unsigned long OrdinalBase)
{
    const char *filename;
    unsigned char *file;
    size_t file_size;
    unsigned char *pos;

    unsigned long lu_timestamp;

    unsigned long linker_member_size;
    unsigned long num_linker_member_offsets;
    unsigned char *offset_pos;
    unsigned char *string_table_pos;
    size_t i;

    filename = ld_state->output_implib_filename;

    if (insert_timestamp) {
        time_t timestamp;
        timestamp = time (NULL);
        lu_timestamp = (unsigned long)timestamp;
    } else lu_timestamp = 0;

    file_size = strlen (IMAGE_ARCHIVE_START);
    linker_member_size = 1 * 4;

    num_linker_member_offsets = 0;
    if (implib_compat) {
        coff_implib_compat_calculate (&file_size,
                                      &num_linker_member_offsets,
                                      &linker_member_size);
    }
    for (i = 0; i < num_names; i++) {
        file_size += SIZEOF_struct_IMAGE_ARCHIVE_MEMBER_HEADER_file;
        file_size += SIZEOF_struct_IMPORT_OBJECT_HEADER_file;
        if (leading_underscore) file_size++;
        file_size += strlen (export_names[i].name) + 1;
        file_size += strlen (ld_state->output_filename) + 1;

        if (export_names[i].export_type == EXPORT_TYPE_CODE) {
            if (leading_underscore) linker_member_size++;
            linker_member_size += 4 + strlen (export_names[i].name) + 1;
            num_linker_member_offsets++;
        }
        if (leading_underscore) linker_member_size++;
        linker_member_size += 4 + IMP_PREFIX_LEN + strlen (export_names[i].name) + 1;
        num_linker_member_offsets++;

        file_size = ALIGN (file_size, 2);
    }

    linker_member_size = ALIGN (linker_member_size, 2);
    file_size += SIZEOF_struct_IMAGE_ARCHIVE_MEMBER_HEADER_file + linker_member_size;

    file = xmalloc (file_size);
    memset (file, 0, file_size);

    pos = file;

    memcpy (pos, IMAGE_ARCHIVE_START, strlen (IMAGE_ARCHIVE_START));
    pos += strlen (IMAGE_ARCHIVE_START);

    write_archive_member_header (pos, IMAGE_ARCHIVE_LINKER_MEMBER_Name, linker_member_size, lu_timestamp);
    pos += SIZEOF_struct_IMAGE_ARCHIVE_MEMBER_HEADER_file;

    bytearray_write_4_bytes (pos, num_linker_member_offsets, BIG_ENDIAN);
    pos += 4;

    offset_pos = pos;
    string_table_pos = pos + num_linker_member_offsets * 4;
    pos = file + strlen (IMAGE_ARCHIVE_START) + SIZEOF_struct_IMAGE_ARCHIVE_MEMBER_HEADER_file + linker_member_size;

    if (implib_compat) {
        coff_implib_compat_write (file,
                                  &pos,
                                  &offset_pos,
                                  &string_table_pos,
                                  lu_timestamp);
    }

    for (i = 0; i < num_names; i++) {

        if (export_names[i].export_type == EXPORT_TYPE_CODE) {
            bytearray_write_4_bytes (offset_pos, pos - file, BIG_ENDIAN);
            offset_pos += 4;
            
            if (leading_underscore) string_table_pos++[0] = '_';
            strcpy ((char *)string_table_pos, export_names[i].name);
            string_table_pos += strlen (export_names[i].name) + 1;
        }

        {
            bytearray_write_4_bytes (offset_pos, pos - file, BIG_ENDIAN);
            offset_pos += 4;
            
            memcpy (string_table_pos, IMP_PREFIX_STR, IMP_PREFIX_LEN);
            string_table_pos += IMP_PREFIX_LEN;
            if (leading_underscore) string_table_pos++[0] = '_';
            strcpy ((char *)string_table_pos, export_names[i].name);
            string_table_pos += strlen (export_names[i].name) + 1;
        }

        /* With --implib-compat,
         * all archive members must be named after the DLL for compatibility
         * because other linkers pointlessly depend on it.
         */
        write_archive_member_header (pos, !implib_compat ? "IMPORT/" : "/0",
                                     SIZEOF_struct_IMPORT_OBJECT_HEADER_file
                                     + (leading_underscore ? 1 : 0)
                                     + strlen (export_names[i].name) + 1
                                     + strlen (ld_state->output_filename) + 1,
                                     lu_timestamp);
        pos += SIZEOF_struct_IMAGE_ARCHIVE_MEMBER_HEADER_file;

        {
            struct IMPORT_OBJECT_HEADER_internal import_hdr;

            import_hdr.Magic1 = IMAGE_FILE_MACHINE_UNKNOWN;
            import_hdr.Magic2 = IMPORT_OBJECT_HDR_MAGIC2;
            import_hdr.Version = 0;
            import_hdr.Machine = get_Machine ();

            import_hdr.TimeDateStamp = lu_timestamp;
            
            import_hdr.SizeOfData = strlen (export_names[i].name) + 1 + strlen (ld_state->output_filename) + 1;
            if (leading_underscore) import_hdr.SizeOfData++;
            import_hdr.OrdinalHint = OrdinalBase + i;
            
            switch (export_names[i].export_type) {
                case EXPORT_TYPE_CODE: import_hdr.Type = IMPORT_CODE; break;
                case EXPORT_TYPE_DATA: import_hdr.Type = IMPORT_DATA; break;
                case EXPORT_TYPE_CONST: import_hdr.Type = IMPORT_CONST; break;
            }

            if (kill_at) import_hdr.Type |= IMPORT_NAME_UNDECORATE << 2;
            else if (leading_underscore) import_hdr.Type |= IMPORT_NAME_NOPREFIX << 2;
            else import_hdr.Type |= IMPORT_NAME << 2;

            write_struct_IMPORT_OBJECT_HEADER (pos, &import_hdr);
            pos += SIZEOF_struct_IMPORT_OBJECT_HEADER_file;

            if (leading_underscore) pos++[0] = '_';
            strcpy ((char *)pos, export_names[i].name);
            pos += strlen (export_names[i].name) + 1;
            strcpy ((char *)pos, ld_state->output_filename);
            pos += strlen (ld_state->output_filename) + 1;
        }

        pos = file + ALIGN (pos - file, 2);
    }

    {
        FILE *outfile;
        
        if (!(outfile = fopen (filename, "wb"))) {
            ld_error ("cannot open '%s' for writing", filename);
            return;
        }

        if (fwrite (file, file_size, 1, outfile) != 1) {
            ld_error ("writing '%s' file failed", filename);
        }

        fclose (outfile);
    }

    free (file);
    return;
}

#undef KILL_AT_PICK_NAME_I

static void generate_edata (void)
{
    struct object_file *of;
    struct section *section;
    struct section_part *part;
    size_t num_names, name_table_size;
    struct export_name *export_names;
    size_t i;
    struct symbol *symbol;
    struct reloc_entry *relocs;
    size_t name_table_offset;
    struct IMAGE_EXPORT_DIRECTORY_internal ied;
    unsigned long OrdinalBase;

    OrdinalBase = 1;

    {
        struct name_list *name_list, *next_name_list;

        for (name_list = export_name_list, num_names = 0, name_table_size = 0;
             name_list;
             name_list = name_list->next) {
            num_names++;
        }

        export_names = xmalloc (sizeof (*export_names) * num_names);
        for (name_list = export_name_list, i = 0;
             name_list;
             name_list = next_name_list, i++) {
            next_name_list = name_list->next;
            
            export_names[i].name = name_list->name;
            if (kill_at) {
                export_names[i].name_no_at = unat_name (export_names[i].name);
                name_table_size += strlen (export_names[i].name_no_at) + 1;
            } else {
                export_names[i].name_no_at = NULL;
                name_table_size += strlen (export_names[i].name) + 1;
            }
                
            export_names[i].export_type = name_list->info ? EXPORT_TYPE_DATA : EXPORT_TYPE_CODE;
            
            free (name_list);
        }
    }

    qsort (export_names, num_names, sizeof (*export_names), &export_name_compar);

    if (ld_state->output_implib_filename) write_implib (export_names, num_names, OrdinalBase);
    
    name_table_size += strlen (ld_state->output_filename) + 1;

    of = object_file_make (1 + num_names, FAKE_LD_FILENAME);
    section = section_find_or_make (".edata");
    section->flags = translate_Characteristics_to_section_flags (IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ);
    part = section_part_new (section, of);

    part->content_size = (SIZEOF_struct_IMAGE_EXPORT_DIRECTORY_file
                          + (num_names
                             * (SIZEOF_struct_EXPORT_Address_Table_file
                                + SIZEOF_struct_EXPORT_Name_Pointer_Table_file
                                + SIZEOF_struct_EXPORT_Ordinal_Table_file))
                          + name_table_size);
    part->content = xmalloc (part->content_size);
    memset (part->content, 0, part->content_size);
    section_append_section_part (section, part);
    
    part->relocation_count = 4 + num_names * 2;
    part->relocation_array = xcalloc (part->relocation_count, sizeof *part->relocation_array);
    relocs = part->relocation_array;

    of->symbol_array[0].name = xstrdup (section->name);
    of->symbol_array[0].value = 0;
    of->symbol_array[0].part = part;
    of->symbol_array[0].section_number = 1;

    name_table_offset = (SIZEOF_struct_IMAGE_EXPORT_DIRECTORY_file
                          + (num_names
                             * (SIZEOF_struct_EXPORT_Address_Table_file
                                + SIZEOF_struct_EXPORT_Name_Pointer_Table_file
                                + SIZEOF_struct_EXPORT_Ordinal_Table_file)));

    {
        ied.ExportFlags = 0;
        if (insert_timestamp) {
            /* This timestamp is going to have different value than the timestamp in the COFF header
             * but that is not forbidden. */
            time_t timestamp;
            timestamp = time (NULL);
            ied.TimeDateStamp = (unsigned long)timestamp;
        } else {
            ied.TimeDateStamp = 0;
        }
        ied.MajorVersion = 0;
        ied.MinorVersion = 0;
        ied.NameRVA = name_table_offset;
        ied.OrdinalBase = OrdinalBase;
        ied.AddressTableEntries = num_names;
        ied.NumberOfNamePointers = num_names;
        ied.ExportAddressTableRVA = SIZEOF_struct_IMAGE_EXPORT_DIRECTORY_file;
        ied.NamePointerRVA = ied.ExportAddressTableRVA + num_names * SIZEOF_struct_EXPORT_Address_Table_file;
        ied.OrdinalTableRVA = ied.NamePointerRVA + num_names * SIZEOF_struct_EXPORT_Name_Pointer_Table_file;

        write_struct_IMAGE_EXPORT_DIRECTORY (part->content, &ied);

        for (i = 0; i < 4; i++) {
            relocs[i].symbol = &of->symbol_array[0];
            relocs[i].howto = &reloc_howtos[RELOC_TYPE_32_NO_BASE];
        }
        relocs[0].offset = offsetof (struct IMAGE_EXPORT_DIRECTORY_file, NameRVA);
        relocs[1].offset = offsetof (struct IMAGE_EXPORT_DIRECTORY_file, ExportAddressTableRVA);
        relocs[2].offset = offsetof (struct IMAGE_EXPORT_DIRECTORY_file, NamePointerRVA);
        relocs[3].offset = offsetof (struct IMAGE_EXPORT_DIRECTORY_file, OrdinalTableRVA);
        relocs += 4;
    }

    symbol = of->symbol_array + 1;
    strcpy ((char *)(part->content + name_table_offset), ld_state->output_filename);
    ttgtchsmem(part->content + name_table_offset, strlen (ld_state->output_filename));
    name_table_offset += strlen (ld_state->output_filename) + 1;
    for (i = 0; i < num_names; i++) {
        {
            if (leading_underscore) {
                symbol->name = xmalloc (1 + strlen (export_names[i].name) + 1);
                symbol->name[0] = '_';
                strcpy (symbol->name + 1, export_names[i].name);
            } else {
                symbol->name = xstrdup (export_names[i].name);
            }
            symbol->value = 0;
            symbol->part = NULL;
            symbol->section_number = 0;
            symbol_record_external_symbol (symbol);
            
            relocs[0].offset = ied.ExportAddressTableRVA + SIZEOF_struct_EXPORT_Address_Table_file * i;
            relocs[0].symbol = symbol;
            relocs[0].howto = &reloc_howtos[RELOC_TYPE_32_NO_BASE];

            symbol++;
            relocs++;
        }
        {
            struct EXPORT_Name_Pointer_Table_internal npt;
            npt.FunctionNameRVA = name_table_offset;
            write_struct_EXPORT_Name_Pointer_Table (part->content + ied.NamePointerRVA
                                                    + SIZEOF_struct_EXPORT_Name_Pointer_Table_file * i,
                                                    &npt);
            relocs[0].offset = ied.NamePointerRVA + SIZEOF_struct_EXPORT_Name_Pointer_Table_file * i;
            relocs[0].symbol = &of->symbol_array[0];
            relocs[0].howto = &reloc_howtos[RELOC_TYPE_32_NO_BASE];
            relocs++;
        }
        {
            struct EXPORT_Ordinal_Table_internal ot;
            ot.FunctionOrdinal = i;
            write_struct_EXPORT_Ordinal_Table (part->content + ied.OrdinalTableRVA
                                               + SIZEOF_struct_EXPORT_Ordinal_Table_file * i,
                                               &ot);
        }
        {
            const char *name;
            name = kill_at ? (export_names[i].name_no_at) : (export_names[i].name);
            strcpy ((char *)(part->content + name_table_offset), name);
            ttgtchsmem(part->content + name_table_offset, strlen (name));
            name_table_offset += strlen (name) + 1;
        }
    }

    for (i = 0; i < num_names; i++) {
        free (export_names[i].name);
        free (export_names[i].name_no_at);
    }
    free (export_names);
}

static int check_reloc_section_needed_section_part (const struct section_part *part)
{
    size_t i;
    
    for (i = 0; i < part->relocation_count; i++) {
        if (part->relocation_array[i].howto == &reloc_howtos[RELOC_TYPE_64]
            || part->relocation_array[i].howto == &reloc_howtos[RELOC_TYPE_32]
            || part->relocation_array[i].howto == &reloc_howtos[RELOC_TYPE_ARM_32]
            || part->relocation_array[i].howto == &reloc_howtos[RELOC_TYPE_ARM_MOV32]
            || part->relocation_array[i].howto == &reloc_howtos[RELOC_TYPE_ARM_THUMB_MOV32]) {
            return 1;
        }
    }
    
    return 0;
}

static int check_reloc_section_needed (void)
{
    struct section *section;

    for (section = all_sections; section; section = section->next) {
        struct subsection *subsection;
        struct section_part *part;

        for (part = section->first_part; part; part = part->next) {
            if (check_reloc_section_needed_section_part (part)) return 1;
        }

        for (subsection = section->all_subsections; subsection; subsection = subsection->next) {
            for (part = subsection->first_part; part; part = part->next) {
                if (check_reloc_section_needed_section_part (part)) return 1;
            }
        }
    }

    return 0;
}

void coff_before_link (void)
{
    struct section *section;
    struct subsection *subsection;
    struct section_part *part;
    size_t i;

    if (ld_state->base_address % 0x10000) {
        /* Other output formats set their base address using --image-base too
         * and have no restrictions on it, so to avoid printing unnecessary warning
         * (when --image-base is used before --oformat),
         * the warning was moved here.
         */
        ld_warn ("base address must be a multiple of 64 KiB (0x10000) according to the specification");
    }

    /* If no COFF objects were used as input, leading_underscore must be checked here. */
    check_Machine (get_Machine (), "INTERNAL");

    if (export_all_symbols) export_all_generate_names ();

    exclude_symbols_free ();

    if (export_name_list) {
        generate_edata ();
    } else if (ld_state->output_implib_filename) {
        write_implib (NULL, 0, 0);
    }

    /* Certain OS rejects executables with empty .reloc section,
     * so empty .reloc section must NOT be generated. */
    if (!check_reloc_section_needed ()) {
        can_be_relocated = 1;
        generate_reloc_section = 0;
    }

    if (generate_reloc_section) {
        can_be_relocated = 1;
        section = section_find_or_make (".reloc");
        section->flags = translate_Characteristics_to_section_flags (IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE);
    }

    for (section = all_sections; section; section = section->next) {
        if (section->section_alignment < SectionAlignment) {
            section->section_alignment = SectionAlignment;
        }
    }

    if (stub_file) {
        size_of_headers = stub_size;
    } else {
        size_of_headers = SIZEOF_struct_IMAGE_DOS_HEADER_file;
    }

    size_of_headers += 4 /* "PE\0\0" */ + SIZEOF_struct_coff_header_file;

    if (ld_state->target_machine == LD_TARGET_MACHINE_X64
        || ld_state->target_machine == LD_TARGET_MACHINE_AARCH64) {
        size_of_headers += SIZEOF_struct_optional_header_plus_file;
    } else {
        size_of_headers += SIZEOF_struct_optional_header_file;
    }
    
    size_of_headers += NUMBER_OF_DATA_DIRECTORIES * SIZEOF_struct_IMAGE_DATA_DIRECTORY_file;
    size_of_headers += SIZEOF_struct_section_table_entry_file * section_count ();
    size_of_headers = ALIGN (size_of_headers, FileAlignment);

    /* 1-based section index is needed for IMAGE_REL_I386_SECTION relocations. */
    for (section = all_sections, i = 1; section; section = section->next, i++) {
        section->target_index = i;
    }

    /* .idata$2 contains Import Directory Table which needs to be terminated with null entry. */
    section = section_find (".idata");
    if (section == NULL) return;
    subsection = subsection_find (section, "2");
    if (subsection == NULL) return;
    
    part = section_part_new (section, object_file_make (0, FAKE_LD_FILENAME));

    part->content_size = SIZEOF_struct_IMPORT_Directory_Table_file;
    part->content = xmalloc (part->content_size);
    memset (part->content, 0, part->content_size);

    subsection_append_section_part (subsection, part);

    /* .idata$5 is Import Address Table. */
    subsection = subsection_find (section, "5");
    if (subsection == NULL) return;

    iat_first_part = subsection->first_part;
    if (iat_first_part == NULL) return;
    for (part = iat_first_part; part->next; part = part->next) ;
    iat_last_part = part;
}

static void translate_relocation_64 (struct reloc_entry *reloc,
                                     struct relocation_entry_internal *input_reloc,
                                     struct section_part *part)
{
    reloc->symbol = part->of->symbol_array + input_reloc->SymbolTableIndex;
    reloc->offset = input_reloc->VirtualAddress;

    switch (input_reloc->Type) {
        case IMAGE_REL_AMD64_ABSOLUTE: reloc->howto = &reloc_howtos[RELOC_TYPE_IGNORED]; break;

        case IMAGE_REL_AMD64_ADDR64: reloc->howto = &reloc_howtos[RELOC_TYPE_64]; break;

        case IMAGE_REL_AMD64_ADDR32: reloc->howto = &reloc_howtos[RELOC_TYPE_32]; break;

        case IMAGE_REL_AMD64_ADDR32NB: reloc->howto = &reloc_howtos[RELOC_TYPE_32_NO_BASE]; break;

        case IMAGE_REL_AMD64_REL32: reloc->howto = &reloc_howtos[RELOC_TYPE_PC32]; break;
        
        case IMAGE_REL_AMD64_REL32_1:
        case IMAGE_REL_AMD64_REL32_2:
        case IMAGE_REL_AMD64_REL32_3:
        case IMAGE_REL_AMD64_REL32_4:
        case IMAGE_REL_AMD64_REL32_5:
            reloc->howto = &reloc_howtos[RELOC_TYPE_PC32];
            /* It seems that the "_N" part should be subtracted from the field
             * but it is not clear why that could not be done by the assembler instead.
             */
            {
                unsigned long field;
                
                bytearray_read_4_bytes (&field, part->content + reloc->offset, LITTLE_ENDIAN);
                field -= input_reloc->Type - IMAGE_REL_AMD64_REL32;
                bytearray_write_4_bytes (part->content + reloc->offset, field, LITTLE_ENDIAN);
            }
            break;
        
        case IMAGE_REL_AMD64_SECTION:
        case IMAGE_REL_AMD64_SECREL:
        case IMAGE_REL_AMD64_SECREL7:
        case IMAGE_REL_AMD64_TOKEN:
        case IMAGE_REL_AMD64_SREL32:
        case IMAGE_REL_AMD64_PAIR:
        case IMAGE_REL_AMD64_SSPAN32:
            ld_internal_error_at_source (__FILE__, __LINE__, "+++relocation type 0x%04hx not supported yet", input_reloc->Type);
            break;

        default:
            /* There is no point in continuing, the object is broken. */
            ld_fatal_error ("invalid relocation type 0x%04hx (origin object '%s')", input_reloc->Type, part->of->filename);
            break;
        
    }
}

static void translate_relocation_arm (struct reloc_entry *reloc,
                                      struct relocation_entry_internal *input_reloc,
                                      struct section_part *part)
{
    reloc->symbol = part->of->symbol_array + input_reloc->SymbolTableIndex;
    reloc->offset = input_reloc->VirtualAddress;

    switch (input_reloc->Type) {
        case IMAGE_REL_ARM_ABSOLUTE: reloc->howto = &reloc_howtos[RELOC_TYPE_IGNORED]; break;

        case IMAGE_REL_ARM_ADDR32: reloc->howto = &reloc_howtos[RELOC_TYPE_ARM_32]; break;

        case IMAGE_REL_ARM_ADDR32NB: reloc->howto = &reloc_howtos[RELOC_TYPE_32_NO_BASE]; break;

        /* Revision 8.2 of the specification from 2010-09-21
         * says this should be the most significant 24 bits
         * of 26-bit signed relative displacement
         * but revision 11 (2017-01-23) and later say
         * it is just 24-bit relative displacement.
         */
        case IMAGE_REL_ARM_BRANCH24: reloc->howto = &reloc_howtos[RELOC_TYPE_ARM_PC26]; break;

        case IMAGE_REL_ARM_MOV32: reloc->howto = &reloc_howtos[RELOC_TYPE_ARM_MOV32]; break;
        
        case IMAGE_REL_THUMB_MOV32: reloc->howto = &reloc_howtos[RELOC_TYPE_ARM_THUMB_MOV32]; break;

        case IMAGE_REL_THUMB_BRANCH20: reloc->howto = &reloc_howtos[RELOC_TYPE_ARM_THUMB_BRANCH20]; break;

        case IMAGE_REL_THUMB_BRANCH24: /* Seems to be the same as BLX23. */
        case IMAGE_REL_THUMB_BLX23: reloc->howto = &reloc_howtos[RELOC_TYPE_ARM_THUMB_BLX23]; break;
        
        case IMAGE_REL_ARM_BRANCH11:
        case IMAGE_REL_ARM_SECTION:
        case IMAGE_REL_ARM_SECREL:
        case IMAGE_REL_ARM_PAIR:
            ld_internal_error_at_source (__FILE__, __LINE__, "+++relocation type 0x%04hx not supported yet", input_reloc->Type);
            break;
        
        default:
            /* There is no point in continuing, the object is broken. */
            ld_fatal_error ("invalid relocation type 0x%04hx (origin object '%s')", input_reloc->Type, part->of->filename);
            break;
        
    }
}

static void translate_relocation_arm64 (struct reloc_entry *reloc,
                                        struct relocation_entry_internal *input_reloc,
                                        struct section_part *part)
{
    reloc->symbol = part->of->symbol_array + input_reloc->SymbolTableIndex;
    reloc->offset = input_reloc->VirtualAddress;

    switch (input_reloc->Type) {
        case IMAGE_REL_ARM64_ABSOLUTE: reloc->howto = &reloc_howtos[RELOC_TYPE_IGNORED]; break;

        case IMAGE_REL_ARM64_ADDR32: reloc->howto = &reloc_howtos[RELOC_TYPE_32]; break;

        case IMAGE_REL_ARM64_ADDR32NB: reloc->howto = &reloc_howtos[RELOC_TYPE_32_NO_BASE]; break;

        case IMAGE_REL_ARM64_BRANCH26: reloc->howto = &reloc_howtos[RELOC_TYPE_AARCH64_CALL26]; break;

        case IMAGE_REL_ARM64_PAGEBASE_REL21:
            reloc->howto = &reloc_howtos[RELOC_TYPE_AARCH64_ADR_PREL_PG_HI21];
            /* The addend for this COFF relocation
             * that is already present should not be used,
             * so it must be cleared before doing the relocation.
             */
            {
                unsigned long field;

                bytearray_read_4_bytes (&field, part->content + reloc->offset, LITTLE_ENDIAN);
                field &= ~(address_type)reloc->howto->dst_mask;
                bytearray_write_4_bytes (part->content + reloc->offset, field, LITTLE_ENDIAN);
            }
            break;

        case IMAGE_REL_ARM64_PAGEOFFSET_12A: reloc->howto = &reloc_howtos[RELOC_TYPE_AARCH64_ADD_ABS_LO12_NC]; break;

        case IMAGE_REL_ARM64_PAGEOFFSET_12L:
            /* COFF, unlike ELF, has only one relocation type for "str"-like instructions,
             * what means the linker must decide how to relocate the field on its own
             * by reading the field and identifying the instruction.
             */
            if (part->content[reloc->offset + 3] == 0x39) {
                /* strb w... */
                reloc->howto = &reloc_howtos[RELOC_TYPE_AARCH64_LDST8_ABS_LO12_NC];
            } else if (part->content[reloc->offset + 3] == 0x79) {
                /* strh w... */
                reloc->howto = &reloc_howtos[RELOC_TYPE_AARCH64_LDST16_ABS_LO12_NC]; /*strh*/
            } else if (part->content[reloc->offset + 3] == 0xB9) {
                /* str w... */
                reloc->howto = &reloc_howtos[RELOC_TYPE_AARCH64_LDST32_ABS_LO12_NC];
            } else if (part->content[reloc->offset + 3] == 0xF9) {
                /* str x... */
                reloc->howto = &reloc_howtos[RELOC_TYPE_AARCH64_LDST64_ABS_LO12_NC];
            } else {
                ld_internal_error_at_source (__FILE__, __LINE__,
                                             "unknown instruction '%02x %02x %02x %02x' used with IMAGE_REL_ARM64_PAGEOFFSET_12L",
                                             part->content[reloc->offset],
                                             part->content[reloc->offset + 1],
                                             part->content[reloc->offset + 2],
                                             part->content[reloc->offset + 3]);
            }
            break;

        case IMAGE_REL_ARM64_ADDR64: reloc->howto = &reloc_howtos[RELOC_TYPE_64]; break;

        case IMAGE_REL_ARM64_REL21:
        case IMAGE_REL_ARM64_SECREL:
        case IMAGE_REL_ARM64_SECREL_LOW12A:
        case IMAGE_REL_ARM64_SECREL_HIGH12A:
        case IMAGE_REL_ARM64_SECREL_LOW12L:
        case IMAGE_REL_ARM64_TOKEN:
        case IMAGE_REL_ARM64_SECTION:
        case IMAGE_REL_ARM64_BRANCH19:
        case IMAGE_REL_ARM64_BRANCH14:
            ld_internal_error_at_source (__FILE__, __LINE__, "+++relocation type 0x%04hx not supported yet", input_reloc->Type);
            break;

        default:
            /* There is no point in continuing, the object is broken. */
            ld_fatal_error ("invalid relocation type 0x%04hx (origin object '%s')", input_reloc->Type, part->of->filename);
            break;
        
    }
}

static void translate_relocation (struct reloc_entry *reloc,
                                  struct relocation_entry_internal *input_reloc,
                                  struct section_part *part)
{
    switch (ld_state->target_machine) {
        case LD_TARGET_MACHINE_I386: break;
        
        case LD_TARGET_MACHINE_X64:
            translate_relocation_64 (reloc, input_reloc, part);
            return;
        
        case LD_TARGET_MACHINE_ARM:
            translate_relocation_arm (reloc, input_reloc, part);
            return;

        case LD_TARGET_MACHINE_AARCH64:
            translate_relocation_arm64 (reloc, input_reloc, part);
            return;

        default:
            ld_internal_error_at_source (__FILE__, __LINE__,
                                         "+++relocations for Machine %#x not supported yet",
                                         get_Machine ());
    }

    reloc->symbol = part->of->symbol_array + input_reloc->SymbolTableIndex;
    reloc->offset = input_reloc->VirtualAddress;

    switch (input_reloc->Type) {
        case IMAGE_REL_I386_ABSOLUTE: reloc->howto = &reloc_howtos[RELOC_TYPE_IGNORED]; break;

        case IMAGE_REL_I386_DIR32: reloc->howto = &reloc_howtos[RELOC_TYPE_32]; break;

        case IMAGE_REL_I386_DIR32NB: reloc->howto = &reloc_howtos[RELOC_TYPE_32_NO_BASE]; break;

        case IMAGE_REL_I386_SECTION: reloc->howto = &reloc_howtos[RELOC_TYPE_16_SECTION_INDEX]; break;

        case IMAGE_REL_I386_SECREL: reloc->howto = &reloc_howtos[RELOC_TYPE_I386_SECREL]; break;

        case IMAGE_REL_I386_REL32: reloc->howto = &reloc_howtos[RELOC_TYPE_PC32]; break;

        case IMAGE_REL_I386_TOKEN:
        case IMAGE_REL_I386_SECREL7:
            ld_internal_error_at_source (__FILE__, __LINE__, "+++relocation type 0x%04hx not supported yet", input_reloc->Type);
            break;

        case IMAGE_REL_I386_DIR16:
        case IMAGE_REL_I386_REL16:
        case IMAGE_REL_I386_SEG12:
            /* There is no point in continuing, the user is using very outdated objects. */
            ld_fatal_error ("relocation type 0x%04hx is no longer supported according to the specification", input_reloc->Type);
            break;

        default:
            /* There is no point in continuing, the object is broken. */
            ld_fatal_error ("invalid relocation type 0x%04hx (origin object '%s')", input_reloc->Type, part->of->filename);
            break;

    }
}

address_type coff_calculate_entry_point (void)
{
    const struct symbol *symbol;

    symbol = symbol_find (leading_underscore ? "_mainCRTStartup" : "mainCRTStartup");
    if (symbol) return symbol_get_value_no_base (symbol);

    return 0;
}

#define FLOOR_TO(to_floor, floor) ((to_floor) / (floor) * (floor))

static void generate_base_relocation_block (struct section *reloc_section,
                                            struct IMAGE_BASE_RELOCATION_internal *ibr_hdr_p,
                                            unsigned long num_relocs,
                                            struct section *saved_section,
                                            struct section_part *saved_part,
                                            size_t saved_i)
{
    struct section_part *reloc_part;
    unsigned char *write_pos;
    struct section *section;

    /* There must be even number of Base relocation WORDs
     * because the start of the blocks must be aligned on 4 byte boundary. */
    ibr_hdr_p->SizeOfBlock = ALIGN (SIZEOF_struct_IMAGE_BASE_RELOCATION_file + num_relocs * 2, 4);

    reloc_part = section_part_new (reloc_section, object_file_make (0, FAKE_LD_FILENAME));
    reloc_part->content_size = ibr_hdr_p->SizeOfBlock;
    reloc_part->content = xmalloc (reloc_part->content_size);
    reloc_part->content[reloc_part->content_size - 2] = reloc_part->content[reloc_part->content_size - 1] = 0;

    write_struct_IMAGE_BASE_RELOCATION (reloc_part->content, ibr_hdr_p);
    write_pos = reloc_part->content + SIZEOF_struct_IMAGE_BASE_RELOCATION_file;

    for (section = saved_section; section; section = section->next) {
        struct section_part *part;

        for (part = ((section == saved_section) ? saved_part : section->first_part); part; part = part->next) {
            struct reloc_entry *relocs;
            size_t i;

            relocs = part->relocation_array;
            for (i = ((part == saved_part) ? saved_i : 0); i < part->relocation_count; i++) {
                unsigned short base_relocation_type;
                
                if (relocs[i].howto == &reloc_howtos[RELOC_TYPE_64]) {
                    base_relocation_type = IMAGE_REL_BASED_DIR64;
                } else if (relocs[i].howto == &reloc_howtos[RELOC_TYPE_32]
                           || relocs[i].howto == &reloc_howtos[RELOC_TYPE_ARM_32]) {
                    base_relocation_type = IMAGE_REL_BASED_HIGHLOW;
                } else if (relocs[i].howto == &reloc_howtos[RELOC_TYPE_ARM_MOV32]) {
                    base_relocation_type = IMAGE_REL_BASED_ARM_MOV32;
                } else if (relocs[i].howto == &reloc_howtos[RELOC_TYPE_ARM_THUMB_MOV32]) {
                    base_relocation_type = IMAGE_REL_BASED_THUMB_MOV32;
                } else {
                    continue;
                }

                {
                    unsigned short rel_word;

                    rel_word = (part->rva + relocs[i].offset - ibr_hdr_p->RVAOfBlock) & 0xfff;
                    rel_word |= base_relocation_type << 12;

                    bytearray_write_2_bytes (write_pos, rel_word, LITTLE_ENDIAN);
                    write_pos += 2;
                }
                if (!--num_relocs) goto finish;
            }
        }
    }

    ld_internal_error_at_source (__FILE__, __LINE__, "num_reloc mismatch while generating .reloc section");
    
finish:
    section_append_section_part (reloc_section, reloc_part);
    reloc_section->total_size += reloc_part->content_size;
}

void coff_after_link (void)
{
    struct IMAGE_BASE_RELOCATION_internal ibr_hdr;
    unsigned long num_relocs;
    struct section *saved_section;
    struct section_part *saved_part;
    size_t saved_i;
    struct section *reloc_section;
    struct section *section;

    if (!generate_reloc_section) return;

    if ((reloc_section = section_find (".reloc")) == NULL) {
        ld_internal_error_at_source (__FILE__, __LINE__, ".reloc section could not be found");
    }

    ibr_hdr.RVAOfBlock = 0;
    num_relocs = 0;
    saved_section = NULL;
    saved_part = NULL;
    saved_i = 0;

    for (section = all_sections; section; section = section->next) {
        struct section_part *part;

        for (part = section->first_part; part; part = part->next) {
            struct reloc_entry *relocs;
            size_t i;

            relocs = part->relocation_array;
            for (i = 0; i < part->relocation_count; i++) {
                if (relocs[i].howto != &reloc_howtos[RELOC_TYPE_64]
                    && relocs[i].howto != &reloc_howtos[RELOC_TYPE_32]
                    && relocs[i].howto != &reloc_howtos[RELOC_TYPE_ARM_32]
                    && relocs[i].howto != &reloc_howtos[RELOC_TYPE_ARM_MOV32]
                    && relocs[i].howto != &reloc_howtos[RELOC_TYPE_ARM_THUMB_MOV32]) continue;

                /* Relocations can be listed in any order, so it must not be assumed they have ascending RVAs.
                 * This workaround sometimes produces suboptimal results but the results should be always correct.
                 * Probably better solution would be creating a list of relocations and sorting it by RVA. */
                if (num_relocs
                    && (part->rva + relocs[i].offset >= ibr_hdr.RVAOfBlock + BASE_RELOCATION_PAGE_SIZE
                        || part->rva + relocs[i].offset < ibr_hdr.RVAOfBlock)) {
                    generate_base_relocation_block (reloc_section,
                                                    &ibr_hdr,
                                                    num_relocs,
                                                    saved_section,
                                                    saved_part,
                                                    saved_i);
                    num_relocs = 0;
                }

                if (num_relocs == 0) {
                    ibr_hdr.RVAOfBlock = FLOOR_TO (part->rva + relocs[i].offset, BASE_RELOCATION_PAGE_SIZE);
                    saved_section = section;
                    saved_part = part;
                    saved_i = i;
                }

                num_relocs++;                    
            }
        }
    }

    if (num_relocs) {
        generate_base_relocation_block (reloc_section,
                                        &ibr_hdr,
                                        num_relocs,
                                        saved_section,
                                        saved_part,
                                        saved_i);
    }
}

static unsigned long calculate_checksum (const unsigned char *data, size_t checksum_offset, size_t data_size)
{
    unsigned long sum = 0;
    size_t i;

    for (i = 0; i < data_size / 4; i++) {
        unsigned long temp;
        int carry;

        if (i == checksum_offset / 4) continue;

        bytearray_read_4_bytes (&temp, data + i * 4, LITTLE_ENDIAN);
        /* ULONG_MAX might be bigger than this value on some systems. */
        carry = sum > 0xFFFFFFFFLU - temp;
        sum += temp;
        sum += carry;
        sum &= 0xFFFFFFFFLU;
    }

    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += sum >> 16;
    sum &= 0xFFFF;

    sum += data_size;
    sum &= 0xFFFFFFFFLU;

    return sum;
}

void coff_write (const char *filename)
{
    FILE *outfile;
    unsigned char *file;
    size_t file_size;
    unsigned char *pos;
    unsigned char *checksum_pos;

    struct IMAGE_DOS_HEADER_internal dos_hdr;
    struct coff_header_internal coff_hdr;
    struct optional_header_internal optional_hdr;
    struct optional_header_plus_internal optional_hdr_plus;

    struct section *section;

    if (!(outfile = fopen (filename, "wb"))) {
        ld_error ("cannot open '%s' for writing", filename);
        return;
    }

    {
        size_t total_section_size_to_write = 0;

        for (section = all_sections; section; section = section->next) {
            if (!section->is_bss) total_section_size_to_write += ALIGN (section->total_size, FileAlignment);
        }

        file_size = size_of_headers + total_section_size_to_write;
    }

    file = xmalloc (file_size);
    memset (file, 0, file_size);

    write_sections (file);

    pos = file;

    if (stub_file) {
        read_struct_IMAGE_DOS_HEADER (&dos_hdr, stub_file);
        
        dos_hdr.OffsetToNewEXEHeader = stub_size;

        write_struct_IMAGE_DOS_HEADER (stub_file, &dos_hdr);
        memcpy (pos, stub_file, stub_size);
        pos += stub_size;
        free (stub_file);
    } else {
        memset (&dos_hdr, 0, sizeof (dos_hdr));

        dos_hdr.Magic[0] = ttgtchs('M');
        dos_hdr.Magic[1] = ttgtchs('Z');

        dos_hdr.SizeOfHeaderInParagraphs = SIZEOF_struct_IMAGE_DOS_HEADER_file / IMAGE_DOS_HEADER_PARAGRAPH_SIZE;

        dos_hdr.OffsetToNewEXEHeader = SIZEOF_struct_IMAGE_DOS_HEADER_file;

        write_struct_IMAGE_DOS_HEADER (pos, &dos_hdr);
        pos += SIZEOF_struct_IMAGE_DOS_HEADER_file;
    }

    memcpy (pos, "PE\0\0", 4);
    ttgtchsmem(pos, 4);
    pos += 4;

    coff_hdr.Machine = get_Machine ();
    coff_hdr.NumberOfSections = section_count ();
    
    if (insert_timestamp) {
        /* Specification says TimeDateStamp should be low 32 bits of time_t
         * even though the meaning of time_t is not portable. */
        time_t timestamp;
        timestamp = time (NULL);
        coff_hdr.TimeDateStamp = (unsigned long)timestamp;
    } else {
        coff_hdr.TimeDateStamp = 0;
    }
    
    coff_hdr.PointerToSymbolTable = 0;
    coff_hdr.NumberOfSymbols = 0;
    coff_hdr.SizeOfOptionalHeader = ((ld_state->target_machine == LD_TARGET_MACHINE_X64
                                      || ld_state->target_machine == LD_TARGET_MACHINE_AARCH64)
                                     ? SIZEOF_struct_optional_header_plus_file
                                     : SIZEOF_struct_optional_header_file);   
    coff_hdr.SizeOfOptionalHeader += NUMBER_OF_DATA_DIRECTORIES * SIZEOF_struct_IMAGE_DATA_DIRECTORY_file;
    
    coff_hdr.Characteristics = IMAGE_FILE_EXECUTABLE_IMAGE | IMAGE_FILE_DEBUG_STRIPPED;
    if (ld_state->target_machine == LD_TARGET_MACHINE_X64
        || ld_state->target_machine == LD_TARGET_MACHINE_AARCH64) {
        coff_hdr.Characteristics |= IMAGE_FILE_LARGE_ADDRESS_AWARE;
    } else {
        coff_hdr.Characteristics |= IMAGE_FILE_32BIT_MACHINE;
    }
    
    if (ld_state->create_shared_library) coff_hdr.Characteristics |= IMAGE_FILE_DLL;
    if (!can_be_relocated) coff_hdr.Characteristics |= IMAGE_FILE_RELOCS_STRIPPED;

    write_struct_coff_header (pos, &coff_hdr);
    pos += SIZEOF_struct_coff_header_file;

    if (ld_state->target_machine == LD_TARGET_MACHINE_X64
        || ld_state->target_machine == LD_TARGET_MACHINE_AARCH64) {
        memset (&optional_hdr_plus, 0, sizeof (optional_hdr_plus));

        optional_hdr_plus.Magic = PE32_PLUS_MAGIC;
        optional_hdr_plus.MajorLinkerVersion = LD_MAJOR_VERSION;
        optional_hdr_plus.MinorLinkerVersion = LD_MINOR_VERSION;
        
        /* Seems that these 3 fields should be rounded up to FileAlignment. */
        optional_hdr_plus.SizeOfCode = ALIGN (size_of_code, FileAlignment);
        optional_hdr_plus.SizeOfInitializedData = ALIGN (size_of_initialized_data, FileAlignment);
        optional_hdr_plus.SizeOfUninitializedData = ALIGN (size_of_uninitialized_data, FileAlignment);

        optional_hdr_plus.AddressOfEntryPoint = ld_state->entry_point;

        optional_hdr_plus.BaseOfCode = base_of_code;

        optional_hdr_plus.ImageBase = ld_state->base_address;
        
        optional_hdr_plus.SectionAlignment = SectionAlignment;
        optional_hdr_plus.FileAlignment = FileAlignment;

        optional_hdr_plus.MajorOperatingSystemVersion = 4;
        optional_hdr_plus.MajorImageVersion = 1;
        optional_hdr_plus.MajorSubsystemVersion = MajorSubsystemVersion;
        optional_hdr_plus.MinorSubsystemVersion = MinorSubsystemVersion;

        if (last_section) optional_hdr_plus.SizeOfImage = ALIGN (last_section->rva + last_section->total_size, SectionAlignment);
        else optional_hdr_plus.SizeOfImage = ALIGN (size_of_headers, SectionAlignment);
        optional_hdr_plus.SizeOfHeaders = size_of_headers;

        checksum_pos = pos + offsetof (struct optional_header_plus_file, CheckSum);

        optional_hdr_plus.Subsystem = Subsystem;
        
        optional_hdr_plus.DllCharacteristics = 0;
        if (nx_compat) optional_hdr_plus.DllCharacteristics |= IMAGE_DLLCHARACTERISTICS_NX_COMPAT;
        if (can_be_relocated) optional_hdr_plus.DllCharacteristics |= IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE;

        optional_hdr_plus.SizeOfStackReserve = SizeOfStackReserve;
        optional_hdr_plus.SizeOfStackCommit = SizeOfStackCommit;
        /* No idea how to determine the following. */
        optional_hdr_plus.SizeOfHeapReserve = 0x100000;
        optional_hdr_plus.SizeOfHeapCommit = 0x1000;

        optional_hdr_plus.NumberOfRvaAndSizes = NUMBER_OF_DATA_DIRECTORIES;

        write_struct_optional_header_plus (pos, &optional_hdr_plus);
        pos += SIZEOF_struct_optional_header_plus_file;
    } else {
        memset (&optional_hdr, 0, sizeof (optional_hdr));

        optional_hdr.Magic = IMAGE_NT_OPTIONAL_HDR32_MAGIC;
        optional_hdr.MajorLinkerVersion = LD_MAJOR_VERSION;
        optional_hdr.MinorLinkerVersion = LD_MINOR_VERSION;
        
        /* Seems that these 3 fields should be rounded up to FileAlignment. */
        optional_hdr.SizeOfCode = ALIGN (size_of_code, FileAlignment);
        optional_hdr.SizeOfInitializedData = ALIGN (size_of_initialized_data, FileAlignment);
        optional_hdr.SizeOfUninitializedData = ALIGN (size_of_uninitialized_data, FileAlignment);

        optional_hdr.AddressOfEntryPoint = ld_state->entry_point;

        optional_hdr.BaseOfCode = base_of_code;
        optional_hdr.BaseOfData = base_of_data;

        optional_hdr.ImageBase = ld_state->base_address;
        optional_hdr.SectionAlignment = SectionAlignment;
        optional_hdr.FileAlignment = FileAlignment;

        optional_hdr.MajorOperatingSystemVersion = 4;
        optional_hdr.MajorImageVersion = 1;
        optional_hdr.MajorSubsystemVersion = MajorSubsystemVersion;
        optional_hdr.MinorSubsystemVersion = MinorSubsystemVersion;

        if (last_section) optional_hdr.SizeOfImage = ALIGN (last_section->rva + last_section->total_size, SectionAlignment);
        else optional_hdr.SizeOfImage = ALIGN (size_of_headers, SectionAlignment);
        optional_hdr.SizeOfHeaders = size_of_headers;

        checksum_pos = pos + offsetof (struct optional_header_file, CheckSum);

        optional_hdr.Subsystem = Subsystem;
        
        optional_hdr.DllCharacteristics = 0;
        if (nx_compat) optional_hdr.DllCharacteristics |= IMAGE_DLLCHARACTERISTICS_NX_COMPAT;
        if (can_be_relocated) optional_hdr.DllCharacteristics |= IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE;

        optional_hdr.SizeOfStackReserve = SizeOfStackReserve;
        optional_hdr.SizeOfStackCommit = SizeOfStackCommit;
        /* No idea how to determine the following. */
        optional_hdr.SizeOfHeapReserve = 0x100000;
        optional_hdr.SizeOfHeapCommit = 0x1000;

        optional_hdr.NumberOfRvaAndSizes = NUMBER_OF_DATA_DIRECTORIES;

        write_struct_optional_header (pos, &optional_hdr);
        pos += SIZEOF_struct_optional_header_file;
    }

    {
        int i;
        
        for (i = 0; i < NUMBER_OF_DATA_DIRECTORIES; i++) {
            struct IMAGE_DATA_DIRECTORY_internal idd = {0};

            switch (i) {

                case 0:
                    /* EXPORT Table. */
                    section = section_find (".edata");
                    if (section) {
                        idd.VirtualAddress = section->rva;
                        idd.Size = section->total_size;
                    }
                    break;

                case 1:
                    /* IMPORT Table. */
                    section = section_find (".idata");
                    if (section) {
                        idd.VirtualAddress = section->rva;
                        idd.Size = section->total_size;
                    }
                    break;

                case 5:
                    /* BASE RELOCATION Table. */
                    section = section_find (".reloc");
                    if (section) {
                        idd.VirtualAddress = section->rva;
                        idd.Size = section->total_size;
                    }
                    break;
                
                case 12:
                    /* IMPORT Address Table. */
                    section = section_find (".idata");
                    if (section && iat_first_part) {
                        idd.VirtualAddress = iat_first_part->rva;
                        idd.Size = iat_last_part->rva + iat_last_part->content_size - iat_first_part->rva;
                    }
                    break;

            }
            write_struct_IMAGE_DATA_DIRECTORY (pos, &idd);
            pos += SIZEOF_struct_IMAGE_DATA_DIRECTORY_file;
        }
    }

    for (section = all_sections; section; section = section->next) {
        struct section_table_entry_internal *hdr = section->object_dependent_data;

        write_struct_section_table_entry (pos, hdr);
        pos += SIZEOF_struct_section_table_entry_file;

        free (hdr);
    }

    bytearray_write_4_bytes (checksum_pos,
                             calculate_checksum (file, checksum_pos - file, file_size),
                             LITTLE_ENDIAN);

    if (fwrite (file, file_size, 1, outfile) != 1) {
        ld_error ("writing '%s' file failed", filename);
    }
    

    free (file);
    fclose (outfile);
    return;
}

#define CHECK_READ(memory_position, size_to_read) \
    do { if (((memory_position) - file + (size_to_read) > file_size) \
             || (memory_position) < file) ld_fatal_error ("%s: corrupted input file", filename); } while (0)

static int interpret_dot_drectve_section (const unsigned char *file,
                                          size_t file_size,
                                          const char *filename,
                                          const unsigned char *pos,
                                          size_t size)
{
    char *temp_buf, *p;
    int ret = 0;

    /* According to specification the content of .drectve should be a string
     * but that cannot be trusted, so NUL is appended. */
    temp_buf = xmalloc (size + 1);
    CHECK_READ (pos, size);
    memcpy (temp_buf, pos, size);
    temp_buf[size] = '\0';
    
    if (pos[0] == 0xEF && pos[1] == 0xBB && pos[2] == 0xBF) {
        ld_internal_error_at_source (__FILE__, __LINE__, "UTF-8 byte order marker not yet supported at the start of .drectve section");
    }
    ftgtchsmem(temp_buf, size);

    p = temp_buf;
    while (*p) {
        while (*p == ' ') p++;
        if (strncmp (p, "-export:", 8) == 0
            || strncmp (p, "/EXPORT:", 8) == 0) {
            char *q;
            char saved_c;
            int data = 0;
            
            p += 8;
            q = strchr (p, ' ');
            if (q == NULL) q = p + strlen (p);
            saved_c = *q;
            *q = '\0';
            {
                /* There is ",data" added to data symbol names,
                 * so this handles it. */
                char *comma;
                
                comma = strchr (p, ',');
                if (comma) {
                    if (strcmp (comma, ",data") == 0
                        || strcmp (comma, ",DATA") == 0) data = 1;
                    else {
                        ld_internal_error_at_source (__FILE__, __LINE__,
                                                     "unsupported comma argument to option -export: '%s'",
                                                     comma);
                    }
                    *comma = '\0';
                }
            }
            /* Sometimes the name might be quoted (not sure why). */
            if (*p == '"') p++;
            if (p[strlen (p) - 1] == '"') p[strlen (p) - 1] = '\0';
            
            {
                struct name_list *name_list;
                name_list = xmalloc (sizeof (*name_list));
                name_list->name = xstrdup (p);
                name_list->info = data;
                name_list->next = NULL;
                *last_export_name_list_p = name_list;
                last_export_name_list_p = &name_list->next;
            }
            *q = saved_c;
            p = q;
        } else if (strncmp (p, "-exclude-symbols:", 17) == 0) {
            char *q;
            char saved_c;
            
            p += 17;
            q = strchr (p, ' ');
            if (q == NULL) q = p + strlen (p);
            saved_c = *q;
            *q = '\0';
            {
                struct exclude_symbol *exclude_symbol;
                exclude_symbol = xmalloc (sizeof *exclude_symbol);
                exclude_symbol->name = xstrdup (p);
                exclude_symbol->next = exclude_symbols;
                exclude_symbols = exclude_symbol;
            }
            *q = saved_c;
            p = q;
        } else if (strncmp (p, "-entry:", 7) == 0
                   || strncmp (p, "/ENTRY:", 7) == 0) {
            char *q;
            char saved_c;

            p += 7;
            q = strchr (p, ' ');
            if (q == NULL) q = p + strlen (p);
            saved_c = *q;
            *q = '\0';
            /* The last entry point overrides all previously specified entry points.
             * Giving a warning about it is PDLD specific.
             */
            if (ld_state->entry_symbol_name) {
                ld_warn ("%s: .drectve option '-entry:' overrides previously specified entry point", filename);
            }
            {
                /* Simple way to ensure the duplicated entry point name gets freed. */
                const struct object_file *fake_of;
                if (leading_underscore) {
                    char *tmp = xmalloc (1 + strlen (p) + 1);
                    tmp[0] = '_';
                    strcpy (tmp + 1, p);
                    fake_of = object_file_make (0, tmp);
                    free (tmp);
                } else {
                    fake_of = object_file_make (0, p);
                }
                /* .drectve "-entry:" should first check all symbols
                 * from the same object file (including local only symbols)
                 * and if no symbol matches, global symbols should be searched.
                 */
                ld_state->entry_symbol_name = fake_of->filename;
                ld_state->entry_local_symbol = NULL;
                ret = 1;
            }
            *q = saved_c;
            p = q;
        } else if (*p) {
            ld_internal_error_at_source (__FILE__, __LINE__, "unsupported .drectve option: %s", p);
        }
    }

    free (temp_buf);
    return ret;
}

union sym_tab_entry {
    struct symbol_table_entry_internal sym;
    unsigned char aux[SIZEOF_struct_symbol_table_entry_file];
};

static union sym_tab_entry *read_symbol_table (unsigned char *file,
                                               size_t file_size,
                                               const char *filename,
                                               const struct coff_header_internal *coff_hdr_p,
                                               unsigned long *comdat_aux_symbol_indexes,
                                               unsigned long *comdat_comdat_symbol_indexes)
{
    union sym_tab_entry *read_symtab;
    unsigned char *pos;
    unsigned long i;
    unsigned char aux_num = 0;

    pos = file + coff_hdr_p->PointerToSymbolTable;
    CHECK_READ (pos, SIZEOF_struct_symbol_table_entry_file * coff_hdr_p->NumberOfSymbols);

    read_symtab = xmalloc (sizeof (*read_symtab) * coff_hdr_p->NumberOfSymbols);
    
    memset (comdat_aux_symbol_indexes, 0, sizeof (*comdat_aux_symbol_indexes) * coff_hdr_p->NumberOfSections);

    for (i = 0; i < coff_hdr_p->NumberOfSymbols; i++, pos += SIZEOF_struct_symbol_table_entry_file) {
        if (aux_num) {
            memcpy (read_symtab[i].aux, pos, sizeof (read_symtab[i].aux));
            aux_num--;
            continue;
        }
        
        read_struct_symbol_table_entry (&read_symtab[i].sym, pos);
        
        aux_num = read_symtab[i].sym.NumberOfAuxSymbols;

        if (read_symtab[i].sym.SectionNumber > 0
            && read_symtab[i].sym.SectionNumber <= coff_hdr_p->NumberOfSections) {
            short sec_num = read_symtab[i].sym.SectionNumber - 1;

            if (!comdat_aux_symbol_indexes[sec_num]) {
                comdat_aux_symbol_indexes[sec_num] = i + 1;
            } else if (!comdat_comdat_symbol_indexes[sec_num]) {
                comdat_comdat_symbol_indexes[sec_num] = i;
            }
        }
    }
    
    if (aux_num) {
        ld_error ("incorrect NumberOfAuxSymbols, exceeds symbol table size");
        free (read_symtab);
        return NULL;
    }    

    return read_symtab;
}

static int read_coff_object (unsigned char *file, size_t file_size, const char *filename)
{
    struct coff_header_internal coff_hdr;
    struct string_table_header_internal string_table_hdr;
    const char *string_table = NULL;
    struct section_table_entry_internal section_hdr;
    union sym_tab_entry *read_symtab = NULL;
    unsigned long *comdat_aux_symbol_indexes;
    unsigned long *comdat_comdat_symbol_indexes;
    struct section_part dummy_comdat_part_s;
    int use_local_entry_point = 0;

    unsigned char *pos, *section_table;

    struct object_file *of;
    struct section_part **part_p_array;
    struct section *bss_section;
    long bss_section_number;
    unsigned long i;

    pos = file;
    CHECK_READ (pos, SIZEOF_struct_coff_header_file);
    read_struct_coff_header (&coff_hdr, pos);

    if (check_Machine (coff_hdr.Machine, filename)) return 1;

    /* IMAGE_FILE_MACHINE_ARMNT uses (new) Thumb mode code
     * while IMAGE_FILE_MACHINE_THUMB actually uses ARM mode code.
     * Only real object files should be used for determing which mode is used,
     * not short import libraries.
     */
    if (coff_hdr.Machine == IMAGE_FILE_MACHINE_ARMNT) {
        if (arm_thumb_mode == 0) ld_warn ("%s: Thumb code when other code is non-Thumb", filename);
        arm_thumb_mode = 1;
    } else if (coff_hdr.Machine == IMAGE_FILE_MACHINE_THUMB) {
        if (arm_thumb_mode == 1) ld_warn ("%s: non-Thumb code when other code is Thumb", filename);
        arm_thumb_mode = 0;
    }

    pos = file + coff_hdr.PointerToSymbolTable + SIZEOF_struct_symbol_table_entry_file * coff_hdr.NumberOfSymbols;
    CHECK_READ (pos, SIZEOF_struct_string_table_header_file);
    read_struct_string_table_header (&string_table_hdr, pos);
    if (string_table_hdr.StringTableSize < 4) ld_error ("invalid string table size: %lu", string_table_hdr.StringTableSize);
    else {
        CHECK_READ (pos, string_table_hdr.StringTableSize);
        string_table = (char *)pos;
    }

    part_p_array = xmalloc (sizeof (*part_p_array) * (coff_hdr.NumberOfSections + 1));
    of = object_file_make (coff_hdr.NumberOfSymbols, filename);
    bss_section = NULL;
    bss_section_number = 0;

    comdat_aux_symbol_indexes = xcalloc (coff_hdr.NumberOfSections, sizeof *comdat_aux_symbol_indexes);
    comdat_comdat_symbol_indexes = xcalloc (coff_hdr.NumberOfSections, sizeof *comdat_comdat_symbol_indexes);
    if (coff_hdr.NumberOfSymbols) {
        read_symtab = read_symbol_table (file, file_size, filename, &coff_hdr, comdat_aux_symbol_indexes, comdat_comdat_symbol_indexes);
        if (read_symtab == NULL) {
            free (comdat_aux_symbol_indexes);
            free (comdat_comdat_symbol_indexes);
            return 1;
        }
    }

    /* Despite PE/COFF specification saying that SizeOfOptionalHeader
     * should be 0 for object files, it is not always followed
     * and needs to be considered (because it is between COFF header and section table).
     */
    section_table = file + SIZEOF_struct_coff_header_file + coff_hdr.SizeOfOptionalHeader;
    for (i = 0; i < coff_hdr.NumberOfSections; i++) {

        pos = section_table + SIZEOF_struct_section_table_entry_file * i;
        CHECK_READ (pos, SIZEOF_struct_section_table_entry_file);
        read_struct_section_table_entry (&section_hdr, pos);

        {
            struct section *section;
            struct subsection *subsection;

            {
                char *section_name;
                char *p;

                section_name = xstrndup (section_hdr.Name, 8);

                if (section_name[0] == '/') {
                    unsigned long offset = 0;
                    
                    offset = strtoul (section_name + 1, NULL, 10);
                    if (offset < string_table_hdr.StringTableSize) {
                        free (section_name);
                        section_name = xstrdup (string_table + offset);
                    } else ld_fatal_error ("%s: invalid offset into string table", filename);
                }

                p = strchr (section_name, '$');
                if (p) {
                    *p = '\0';
                    p++;
                }

                if (section_hdr.Characteristics & IMAGE_SCN_LNK_COMDAT) {
                    struct aux_section_symbol_internal aux_symbol;
                    unsigned long sym_i = comdat_aux_symbol_indexes[i];
                    
                    if (!sym_i) {
                        ld_error ("%s: missing section symbol for COMDAT section '%s'",
                                  filename, section_name);
                        return 1;
                    }
                    
                    if (read_symtab[sym_i - 1].sym.Value != 0
                        || read_symtab[sym_i - 1].sym.Type != IMAGE_SYM_TYPE_NULL
                        || read_symtab[sym_i - 1].sym.StorageClass != IMAGE_SYM_CLASS_STATIC
                        || read_symtab[sym_i - 1].sym.NumberOfAuxSymbols != 1) {
                        ld_error ("%s: invalid section symbol for COMDAT section '%s'",
                                  filename, section_name);
                        return 1;
                    }

                    read_struct_aux_section_symbol (&aux_symbol, read_symtab[sym_i].aux);
                    
                    if (aux_symbol.Length != section_hdr.SizeOfRawData
                        || aux_symbol.NumberOfRelocations != section_hdr.NumberOfRelocations
                        || aux_symbol.NumberOfLinenumbers != section_hdr.NumberOfLinenumbers) {
                        ld_warn ("%s: section auxiliary symbol inconsistent with section header for COMDAT section '%s'",
                                 filename, section_name);
                    }

                    if (aux_symbol.Selection == IMAGE_COMDAT_SELECT_ASSOCIATIVE) {
                        /* IMAGE_COMDAT_SELECT_ASSOCIATIVE is included
                         * when the associated COMDAT section is included
                         * and is discarded when the associated COMDAT section
                         * is discarded.
                         * That means the COMDAT code needs to be run recursively
                         * but only once (because COMDAT chaining is not permitted).
                         * Duplicating the code and using prefix "assoc_" was the easiest.
                         */
                        struct section_table_entry_internal assoc_section_hdr;
                        struct aux_section_symbol_internal assoc_aux_symbol;
                        unsigned long assoc_sym_i;
                        unsigned long assoc_i;
                        
                        const struct symbol *old_comdat_symbol;
                        char *sym_name;
                        struct symbol_table_entry_internal *coff_symbol;

                        if (aux_symbol.Number < 1
                            || aux_symbol.Number > coff_hdr.NumberOfSections) {
                            ld_fatal_error ("%s: invalid COMDAT Number", filename);
                        }

                        assoc_i = aux_symbol.Number - 1;
                        
                        pos = section_table + SIZEOF_struct_section_table_entry_file * assoc_i;
                        CHECK_READ (pos, SIZEOF_struct_section_table_entry_file);
                        read_struct_section_table_entry (&assoc_section_hdr, pos);

                        if (!(section_hdr.Characteristics & IMAGE_SCN_LNK_COMDAT)) {
                            ld_fatal_error ("%s: IMAGE_COMDAT_SELECT_ASSOCIATIVE associated with non-COMDAT section",
                                            filename);
                        }

                        assoc_sym_i = comdat_aux_symbol_indexes[assoc_i];
                        read_struct_aux_section_symbol (&assoc_aux_symbol, read_symtab[assoc_sym_i].aux);

                        if (assoc_aux_symbol.Selection == IMAGE_COMDAT_SELECT_ASSOCIATIVE) {
                            ld_fatal_error ("%s: IMAGE_COMDAT_SELECT_ASSOCIATIVE associated with IMAGE_COMDAT_SELECT_ASSOCIATIVE",
                                            filename);
                        }
                        
                        if (assoc_aux_symbol.Selection != IMAGE_COMDAT_SELECT_ANY
                            && assoc_aux_symbol.Selection != IMAGE_COMDAT_SELECT_NODUPLICATES) {
                            ld_internal_error_at_source (__FILE__, __LINE__,
                                                         "unsupported COMDAT Selection %u", assoc_aux_symbol.Selection);
                        }

                        if (comdat_comdat_symbol_indexes[assoc_i] == 0) {
                            ld_fatal_error ("%s: COMDAT section missing COMDAT symbol",
                                            filename);
                        }

                        coff_symbol = &read_symtab[comdat_comdat_symbol_indexes[assoc_i]].sym;

                        if (memcmp (coff_symbol->Name, "\0\0\0\0", 4) == 0) {
                            unsigned long offset = 0;

                            bytearray_read_4_bytes (&offset, (unsigned char *)(coff_symbol->Name + 4), LITTLE_ENDIAN);

                            if (offset < string_table_hdr.StringTableSize) {
                                sym_name = xstrdup (string_table + offset);
                            } else ld_fatal_error ("%s: invalid offset into string table", filename);
                        } else sym_name = xstrndup (coff_symbol->Name, 8);

                        old_comdat_symbol = symbol_find (sym_name);
                        free (sym_name);
                        if (old_comdat_symbol && symbol_is_undefined (old_comdat_symbol)) {
                            old_comdat_symbol = NULL;
                        }

                        if (old_comdat_symbol
                            && coff_symbol->StorageClass == IMAGE_SYM_CLASS_EXTERNAL) {
                            if (assoc_aux_symbol.Selection == IMAGE_COMDAT_SELECT_NODUPLICATES) {
                                ld_fatal_error ("%s: multiply defined COMDAT symbol '%s', first defined in '%s'",
                                                filename, old_comdat_symbol->name,
                                                old_comdat_symbol->part
                                                ? old_comdat_symbol->part->of->filename
                                                : "UNKNOWN");
                            }
                            part_p_array[i + 1] = &dummy_comdat_part_s;
                            free (section_name);
                            continue;
                        }
                    } else {
                        /* The first symbol with specific section number
                         * is always section symbol with auxiliary symbol.
                         * The second symbol with the same section number
                         * is a regular symbol which identifies the COMDAT section
                         * (the section name and CheckSum do not matter at all).
                         * All sections which define the same regular symbol
                         * (so called "COMDAT symbol") are duplicates of each other.
                         *
                         * Checking for duplicate COMDAT sections can thus be done easily
                         * by looking up the regular symbol ("COMDAT symbol") in global symbol hashtab.
                         */
                        const struct symbol *old_comdat_symbol;
                        char *sym_name;
                        struct symbol_table_entry_internal *coff_symbol;

                        if (comdat_comdat_symbol_indexes[i] == 0) {
                            ld_fatal_error ("%s: COMDAT section missing COMDAT symbol", filename);
                        }

                        coff_symbol = &read_symtab[comdat_comdat_symbol_indexes[i]].sym;

                        if (memcmp (coff_symbol->Name, "\0\0\0\0", 4) == 0) {
                            unsigned long offset = 0;

                            bytearray_read_4_bytes (&offset, (unsigned char *)(coff_symbol->Name + 4), LITTLE_ENDIAN);

                            if (offset < string_table_hdr.StringTableSize) {
                                sym_name = xstrdup (string_table + offset);
                            } else ld_fatal_error ("%s: invalid offset into string table", filename);
                        } else sym_name = xstrndup (coff_symbol->Name, 8);

                        old_comdat_symbol = symbol_find (sym_name);
                        free (sym_name);
                        if (old_comdat_symbol && symbol_is_undefined (old_comdat_symbol)) {
                            old_comdat_symbol = NULL;
                        }
                        
                        if (aux_symbol.Selection != IMAGE_COMDAT_SELECT_ANY
                            && aux_symbol.Selection != IMAGE_COMDAT_SELECT_NODUPLICATES) {
                            ld_internal_error_at_source (__FILE__, __LINE__,
                                                         "unsupported COMDAT Selection %u", aux_symbol.Selection);
                        }

                        /* If the COMDAT symbol is not external,
                         * the COMDAT symbol would not be recorded
                         * in global symbol hashtab anyway,
                         * so just assume the COMDAT section is not actually COMDAT
                         * instead of using only half of COMDAT logic.
                         */
                        if (old_comdat_symbol
                            && coff_symbol->StorageClass == IMAGE_SYM_CLASS_EXTERNAL) {
                            if (aux_symbol.Selection == IMAGE_COMDAT_SELECT_NODUPLICATES) {
                                ld_fatal_error ("%s: multiply defined COMDAT symbol '%s', first defined in '%s'",
                                                filename, old_comdat_symbol->name,
                                                old_comdat_symbol->part
                                                ? old_comdat_symbol->part->of->filename
                                                : "UNKNOWN");
                            }
                            part_p_array[i + 1] = &dummy_comdat_part_s;
                            free (section_name);
                            continue;
                        }
                    }
                }

                if (strcmp (section_name, ".drectve") == 0) {
                    use_local_entry_point |= interpret_dot_drectve_section (file,
                                                                            file_size,
                                                                            filename,
                                                                            file + section_hdr.PointerToRawData,
                                                                            section_hdr.SizeOfRawData);
                    part_p_array[i + 1] = NULL;
                    free (section_name);
                    continue;
                }

                if (section_hdr.Characteristics & IMAGE_SCN_LNK_REMOVE) {
                    /* .drectve can have IMAGE_SCN_LNK_REMOVE set
                     * but it must be interpreted anyway.
                     */
                    part_p_array[i + 1] = NULL;
                    free (section_name);
                    continue;
                }

                section = section_find_or_make (section_name);
                section->flags = translate_Characteristics_to_section_flags (section_hdr.Characteristics);
                
                /* Some object files define section without any data to get symbol
                 * to the beginning of the section when content is read in from other object
                 * but such definition must not make the section BSS. */
                if (section_hdr.PointerToRawData == 0 && section_hdr.SizeOfRawData) {
                    section->is_bss = 1;
                }

                if (p) subsection = subsection_find_or_make (section, p);
                else subsection = NULL;

                free (section_name);
            }

            {
                struct section_part *part = section_part_new (section, of);

                part->alignment = translate_Characteristics_to_alignment (section_hdr.Characteristics);

                part->content_size = section_hdr.SizeOfRawData;
                if (section_hdr.PointerToRawData) {
                    pos = file + section_hdr.PointerToRawData;
                    part->content = xmalloc (part->content_size);

                    CHECK_READ (pos, part->content_size);
                    memcpy (part->content, pos, part->content_size);
                }
                if (section->is_bss) {
                    bss_section_number = i + 1;
                    bss_section = section;
                }

                if (section_hdr.PointerToRelocations && section_hdr.NumberOfRelocations) {

                    size_t j;
                    struct relocation_entry_internal relocation;

                    if (!section_hdr.PointerToRawData) {
                        ld_fatal_error ("section '%s' is BSS but has relocations", section->name);
                    }

                    pos = file + section_hdr.PointerToRelocations;

                    part->relocation_array = xcalloc (section_hdr.NumberOfRelocations, sizeof *part->relocation_array);
                    part->relocation_count = section_hdr.NumberOfRelocations;
                    
                    CHECK_READ (pos, SIZEOF_struct_relocation_entry_file * section_hdr.NumberOfRelocations);
                    for (j = 0; j < section_hdr.NumberOfRelocations; j++) {
			read_struct_relocation_entry (&relocation,
                                                      pos + SIZEOF_struct_relocation_entry_file * j);
                        translate_relocation (part->relocation_array + j, &relocation, part);
                    }
                }
                
                if (subsection) {
                    subsection_append_section_part (subsection, part);
                } else {
                    section_append_section_part (section, part);
                }

                part_p_array[i + 1] = part;
            }
        }
    }

    for (i = 0; i < coff_hdr.NumberOfSymbols; i++) {

        struct symbol_table_entry_internal *coff_symbol = &read_symtab[i].sym;
        struct symbol *symbol = of->symbol_array + i;

        if (memcmp (coff_symbol->Name, "\0\0\0\0", 4) == 0) {

            unsigned long offset = 0;

            bytearray_read_4_bytes (&offset, (unsigned char *)(coff_symbol->Name + 4), LITTLE_ENDIAN);

            if (offset < string_table_hdr.StringTableSize) {
                symbol->name = xstrdup (string_table + offset);
            } else ld_fatal_error ("invalid offset into string table");
            
        } else symbol->name = xstrndup (coff_symbol->Name, 8);

        if (coff_symbol->StorageClass == IMAGE_SYM_CLASS_SECTION) {
            /* Some tools create section symbols with IMAGE_SYM_CLASS_SECTION
             * instead of IMAGE_SYM_CLASS_STATIC.
             * For such symbols all fields except Name and StorageClass
             * might have incorrect values and must be ignored
             * and the section to which they belong must be determined
             * using name only.
             */
            char *p;
            size_t j;

            coff_symbol->SectionNumber = 0;

            p = strchr (symbol->name, '$');
            if (p) {
                *p = '\0';
                p++;
            }
            for (j = 1; j < coff_hdr.NumberOfSections + 1; j++) {
                const struct subsection *subsection;
                struct section_part *part = part_p_array[j];
                if (!part) continue;
                if (strcmp (part->section->name, symbol->name)) continue;
                if (!p) {
                    coff_symbol->SectionNumber = j;
                    break;
                }
                subsection = subsection_find (part->section, p);
                if (!subsection) continue;
                if (&part->next != subsection->last_part_p) continue;
                coff_symbol->SectionNumber = j;
                break;
            }

            if (!coff_symbol->SectionNumber) {
                /* It is possible that the section to which the symbol belongs
                 * is not actually present in the object file
                 * and in that case the section should be created with empty section part.
                 */
                struct section *section;
                struct subsection *subsection;
                struct section_part *part;

                section = section_find_or_make (symbol->name);

                if (p) subsection = subsection_find_or_make (section, p);
                else subsection = NULL;

                part = section_part_new (section, of);

                if (subsection) {
                    subsection_append_section_part (subsection, part);
                } else {
                    section_append_section_part (section, part);
                }

                coff_hdr.NumberOfSections = j;
                part_p_array = xrealloc (part_p_array, sizeof (*part_p_array) * (coff_hdr.NumberOfSections + 1));

                part_p_array[j] = part;
                coff_symbol->SectionNumber = j;
            }
            if (p) p[-1] = '$';
            
            coff_symbol->StorageClass = IMAGE_SYM_CLASS_STATIC;
            coff_symbol->Value = 0;
        }

        if (coff_symbol->StorageClass == IMAGE_SYM_CLASS_STATIC
            && coff_symbol->Value == 0) {
            symbol->flags |= SYMBOL_FLAG_SECTION_SYMBOL;
        }

        if (coff_symbol->SectionNumber > 0
            && coff_symbol->SectionNumber <= coff_hdr.NumberOfSections
            && part_p_array[coff_symbol->SectionNumber] == &dummy_comdat_part_s) {
            /* The COMDAT section was discarded,
             * so all symbols defined there are now undefined. */
            coff_symbol->SectionNumber = IMAGE_SYM_UNDEFINED;
            coff_symbol->Value = 0;
        }
        
        symbol->value = coff_symbol->Value;
        symbol->size = 0;
        symbol->section_number = coff_symbol->SectionNumber;
        
        if (coff_symbol->SectionNumber == IMAGE_SYM_UNDEFINED) {
            if (symbol->value) {
                /* It is a common symbol. */
                struct symbol *old_symbol = symbol_find (symbol->name);

                if (!old_symbol || symbol_is_undefined (old_symbol)) {
                    struct section_part *bss_part;
                    
                    if (bss_section == NULL) {
                        bss_section = section_find_or_make (".bss");
                        bss_section->flags = translate_Characteristics_to_section_flags (IMAGE_SCN_CNT_UNINITIALIZED_DATA | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE);
                        bss_section->is_bss = 1;
                        bss_section_number = coff_hdr.NumberOfSections ? coff_hdr.NumberOfSections : 1;
                    }
                    
                    bss_part = section_part_new (bss_section, of);
                    section_append_section_part (bss_section, bss_part);

                    bss_part->content_size = symbol->size = symbol->value;
                    /* ARM64 cannot access unaligned memory
                     * but there is no way to specify common symbol alignment in COFF,
                     * so simple solution is to set the same alignment as is the size
                     * of the field what deals with "str"-like instructions.
                     * (But there should be nothing needing to exceed SectionAlignment,
                     *  so no need to waste memory aligning large symbols.)
                     */  
                    if (bss_part->content_size > SectionAlignment) {
                        bss_part->alignment = SectionAlignment;
                    } else {
                        bss_part->alignment = bss_part->content_size;
                    }
                    
                    symbol->part = bss_part;
                    symbol->value = 0;
                    symbol->section_number = bss_section_number;
                } else {
                    if (symbol->value > old_symbol->size) {
                        old_symbol->part->content_size = old_symbol->size = symbol->value;
                    }
                    
                    symbol->value = 0;
                    symbol->part = NULL;
                }
            } else {
                symbol->part = NULL;
            }
        } else if (coff_symbol->SectionNumber > 0
                   && coff_symbol->SectionNumber <= coff_hdr.NumberOfSections) {
            symbol->part = part_p_array[coff_symbol->SectionNumber];
            if (coff_hdr.Machine == IMAGE_FILE_MACHINE_ARMNT
                && symbol->part
                && (symbol->part->section->flags & SECTION_FLAG_CODE)) {
                symbol->value |= 1;
            }
        } else if (coff_symbol->SectionNumber == IMAGE_SYM_ABSOLUTE) {
            symbol->section_number = ABSOLUTE_SECTION_NUMBER;
            symbol->part = NULL;
        } else if (coff_symbol->SectionNumber == IMAGE_SYM_DEBUG) {
            symbol->section_number = DEBUG_SECTION_NUMBER;
            symbol->part = NULL;
        } else if (coff_symbol->SectionNumber > coff_hdr.NumberOfSections) {
            ld_error ("%s: invalid SectionNumber %hi for symbol '%s'",
                      filename, coff_symbol->SectionNumber, symbol->name);
            symbol->part = NULL;
        } else ld_internal_error_at_source (__FILE__, __LINE__,
                                            "+++not yet supported symbol SectionNumber: %hi",
                                            coff_symbol->SectionNumber);

        if (coff_symbol->StorageClass == IMAGE_SYM_CLASS_EXTERNAL
            /* Undefined and common symbols do not have to have only IMAGE_SYM_CLASS_EXTERNAL StorageClass,
             * they are allowed to have different StorageClass too. */
            || coff_symbol->SectionNumber == IMAGE_SYM_UNDEFINED) {
            symbol_record_external_symbol (symbol);
        }

        if (use_local_entry_point
            && strcmp (ld_state->entry_symbol_name, symbol->name) == 0
            && !symbol_is_undefined (symbol)) {
            ld_state->entry_local_symbol = symbol;
        }

        if (coff_symbol->NumberOfAuxSymbols) {
            unsigned long j;
            
            for (j = 0; j < coff_symbol->NumberOfAuxSymbols; j++) {
                symbol = of->symbol_array + i + 1 + j;
                memset (symbol, 0, sizeof (*symbol));
                symbol->auxiliary = 1;
            }

            i += coff_symbol->NumberOfAuxSymbols;
        }
    }

    free (comdat_aux_symbol_indexes);
    free (comdat_comdat_symbol_indexes);
    free (read_symtab);
    free (part_p_array);

    return 0;
}

static void import_generate_head (const char *dll_name, const char *filename)
{
    struct object_file *of;
    struct section *section;
    struct subsection *subsection;
    struct section_part *part;

    of = object_file_make (3, filename);
    section = section_find_or_make (".idata");
    section->flags = translate_Characteristics_to_section_flags (IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE);

    subsection = subsection_find_or_make (section, "2");
    part = section_part_new (section, of);
    subsection_append_section_part (subsection, part);

    part->content_size = SIZEOF_struct_IMPORT_Directory_Table_file;
    part->content = xmalloc (part->content_size);

    {
        struct IMPORT_Directory_Table_internal import_dt;
        struct reloc_entry *relocs;
        int i;

        import_dt.ImportNameTableRVA = 0;
        import_dt.TimeDateStamp = 0;
        import_dt.ForwarderChain = 0;
        import_dt.NameRVA = 0;
        import_dt.ImportAddressTableRVA = 0;

        write_struct_IMPORT_Directory_Table (part->content, &import_dt);

        part->relocation_count = 3;
        part->relocation_array = xcalloc (part->relocation_count, sizeof *part->relocation_array);
        relocs = part->relocation_array;

        for (i = 0; i < 3; i++) {
            relocs[i].symbol = &of->symbol_array[i];
            relocs[i].howto = &reloc_howtos[RELOC_TYPE_32_NO_BASE];
        }
        relocs[0].offset = offsetof (struct IMPORT_Directory_Table_file, ImportNameTableRVA);
        relocs[1].offset = offsetof (struct IMPORT_Directory_Table_file, ImportAddressTableRVA);
        relocs[2].offset = offsetof (struct IMPORT_Directory_Table_file, NameRVA);
    }
    
    subsection = subsection_find_or_make (section, "4");
    part = section_part_new (section, of);
    subsection_append_section_part (subsection, part);
    part->content_size = 0;

    of->symbol_array[0].name = xstrdup (".idata$4");
    of->symbol_array[0].value = 0;
    of->symbol_array[0].part = part;
    of->symbol_array[0].section_number = 2;

    subsection = subsection_find_or_make (section, "5");
    part = section_part_new (section, of);
    subsection_append_section_part (subsection, part);
    part->content_size = 0;

    of->symbol_array[1].name = xstrdup (".idata$5");
    of->symbol_array[1].value = 0;
    of->symbol_array[1].part = part;
    of->symbol_array[1].section_number = 3;

    subsection = subsection_find_or_make (section, "7");
    part = section_part_new (section, of);
    subsection_append_section_part (subsection, part);
    part->content_size = ALIGN (strlen (dll_name) + 1, 2);
    part->content = xmalloc (part->content_size);
    memset (part->content, 0, part->content_size);
    strcpy ((char *)(part->content), dll_name);
    ttgtchsmem(part->content, strlen ((char *)part->content));

    of->symbol_array[2].name = xstrdup (".idata$7");
    of->symbol_array[2].value = 0;
    of->symbol_array[2].part = part;
    of->symbol_array[2].section_number = 4;
}

static void import_generate_import (const char *import_name,
                                    short OrdinalHint,
                                    short ImportType,
                                    short ImportNameType,
                                    const char *optional_real_import_name,
                                    const char *filename)
{
    struct section_part *dot_idata5_part;
    struct object_file *of;
    struct section *section;
    struct subsection *subsection;
    struct section_part *part;
    struct reloc_entry *relocs;
    struct symbol *symbol;

    /* 2 symbols are internal for .idata$5 and .idata$6. */
    of = object_file_make (2 + 1 + ((ImportType == IMPORT_CODE) ? 1 : 0), filename);
    symbol = of->symbol_array;

    section = section_find_or_make (".idata");

    subsection = subsection_find_or_make (section, "4");
    part = section_part_new (section, of);
    subsection_append_section_part (subsection, part);
    part->content_size = (ld_state->target_machine == LD_TARGET_MACHINE_X64
                          || ld_state->target_machine == LD_TARGET_MACHINE_AARCH64) ? 8 : 4;
    part->content = xmalloc (part->content_size);
    memset (part->content, 0, part->content_size);
    
    part->relocation_count = 1;
    part->relocation_array = xcalloc (part->relocation_count, sizeof *part->relocation_array);
    relocs = part->relocation_array;
    relocs[0].symbol = &of->symbol_array[1];
    relocs[0].howto = &reloc_howtos[RELOC_TYPE_32_NO_BASE];
    relocs[0].offset = 0;

    subsection = subsection_find_or_make (section, "5");
    part = section_part_new (section, of);
    subsection_append_section_part (subsection, part);
    part->content_size = (ld_state->target_machine == LD_TARGET_MACHINE_X64
                          || ld_state->target_machine == LD_TARGET_MACHINE_AARCH64) ? 8 : 4;
    part->content = xmalloc (part->content_size);
    memset (part->content, 0, part->content_size);

    symbol->name = xstrdup (".idata$5");
    symbol->value = 0;
    symbol->part = part;
    symbol->section_number = 2;
    symbol++;

    part->relocation_count = 1;
    part->relocation_array = xcalloc (part->relocation_count, sizeof *part->relocation_array);
    relocs = part->relocation_array;
    relocs[0].symbol = &of->symbol_array[1];
    relocs[0].howto = &reloc_howtos[RELOC_TYPE_32_NO_BASE];
    relocs[0].offset = 0;

    dot_idata5_part = part;

    subsection = subsection_find_or_make (section, "6");
    part = section_part_new (section, of);
    subsection_append_section_part (subsection, part);

    {
        char *real_import_name;

        switch (ImportNameType) {

            case IMPORT_ORDINAL:
                ld_internal_error_at_source (__FILE__, __LINE__, "IMPORT_ORDINAL is not yet supported");
                break;

            case IMPORT_NAME:
                /* OMF import libraries explicitly write the real import name
                 * instead of using IMPORT_NAME_NOPREFIX or IMPORT_NAME_UNDECORATE.
                 */
                if (optional_real_import_name) {
                    real_import_name = xstrdup (optional_real_import_name);
                } else {
                    real_import_name = xstrdup (import_name);
                }
                break;
            
            case IMPORT_NAME_NOPREFIX:
                real_import_name = unprefix_name (import_name);
                break;

            case IMPORT_NAME_UNDECORATE:
                real_import_name = undecorate_name (import_name);
                break;

            default:
                ld_internal_error_at_source (__FILE__, __LINE__,
                                             "unsupported ImportNameType: %i",
                                             ImportNameType);
                break;
            
        }
        
        part->content_size = ALIGN (2 + strlen (real_import_name) + 1, 2);
        part->content = xmalloc (part->content_size);
        memset (part->content, 0, part->content_size);
        bytearray_write_2_bytes (part->content, OrdinalHint, LITTLE_ENDIAN);
        strcpy ((char *)(part->content + 2), real_import_name);
        ttgtchsmem(part->content + 2, strlen ((char *)part->content + 2));

        free (real_import_name);
    }

    symbol->name = xstrdup (".idata$6");
    symbol->value = 0;
    symbol->part = part;
    symbol->section_number = 3;
    symbol++;

    if (ImportType == IMPORT_CODE) {
        section = section_find_or_make (".text");
        part = section_part_new (section, of);
        section_append_section_part (section, part);

        if (ld_state->target_machine == LD_TARGET_MACHINE_I386
            || ld_state->target_machine == LD_TARGET_MACHINE_X64) {
            part->content_size = 8;
            part->content = xmalloc (part->content_size);
            memcpy (part->content,
                    "\xFF\x25\x00\x00\x00\x00" /* jmp dword ptr [0] */
                    "\x90" /* nop */
                    "\x90" /* nop */,
                    8);
        } else if (ld_state->target_machine == LD_TARGET_MACHINE_ARM) {
            part->content_size = 12;
            part->content = xmalloc (part->content_size);

            /* Code for ARM mode is different from code for Thumb mode. */
            if (arm_thumb_mode) {
                memcpy (part->content,
                        "\x40\xF2\x00\x0C" /* movw r12,0x0 */
                        "\xC0\xF2\x00\x0C" /* movt r12,0x0 */
                        "\xDC\xF8\x00\xF0" /* ldr pc,[r12] */,
                        12);
            } else {
                memcpy (part->content,
                        "\x00\xC0\x00\xE3" /* movw r12,0x0 */
                        "\x00\xC0\x40\xE3" /* movt r12,0x0 */
                        "\x00\xF0\x9C\xE5" /* ldr pc,[r12] */,
                        12);
            }
        } else if (ld_state->target_machine == LD_TARGET_MACHINE_AARCH64) {
            part->content_size = 12;
            part->content = xmalloc (part->content_size);

            memcpy (part->content,
                    "\x10\x00\x00\x90" /* adrp x16, #0x0 */
                    "\x10\x02\x40\xF9" /* ldr x16, [x16, #0x0] */
                    "\x00\x02\x1F\xD6" /* br x16 */,
                    12);
        } else if (ld_state->target_machine == LD_TARGET_MACHINE_M68K) {
            part->content_size = 8;
            part->content = xmalloc (part->content_size);
            memcpy (part->content,
                    "\x2F\x39\x00\x00\x00\x00" /* move.l 0.l,-(sp) */
                    "\x4E\x75"                 /* rts */
                    /* (unused) "\x4E\x71" */  /* nop */,
                    8);
        } else if (ld_state->target_machine == LD_TARGET_MACHINE_MAINFRAME) {
            part->content_size = 12;
            part->content = xmalloc (part->content_size);
            memcpy (part->content,
                                       /* USING *,15 */
                    "\x58\xF0\xF0\x08" /* L R15,XXX */
                    "\x07\xFF"         /* BR R15 */
                    "\x07\x00"         /* NOPR R0 */
                    "\x00\x00\x00\x00" /* XXX DC F'0'*/,
                    12);
        } else {
            ld_internal_error_at_source (__FILE__, __LINE__,
                                         "generating PE/COFF import stubs is not yet supported for machine: %i",
                                         ld_state->target_machine);
        }

        symbol->name = xstrdup (import_name);
        symbol->value = 0;
        symbol->part = part;
        symbol->section_number = 4;
        symbol_record_external_symbol (symbol);
        symbol++;

        part->relocation_count = (ld_state->target_machine == LD_TARGET_MACHINE_AARCH64) ? 2 : 1;
        part->relocation_array = xcalloc (part->relocation_count, sizeof *part->relocation_array);
        relocs = part->relocation_array;
        relocs[0].symbol = &of->symbol_array[0];
        if (ld_state->target_machine == LD_TARGET_MACHINE_X64) {
            relocs[0].howto = &reloc_howtos[RELOC_TYPE_PC32];
            relocs[0].offset = 2;
        } else if (ld_state->target_machine == LD_TARGET_MACHINE_I386) {
            relocs[0].howto = &reloc_howtos[RELOC_TYPE_32];
            relocs[0].offset = 2;
        } else if (ld_state->target_machine == LD_TARGET_MACHINE_ARM) {
            if (arm_thumb_mode) {
                relocs[0].howto = &reloc_howtos[RELOC_TYPE_ARM_THUMB_MOV32];
            } else {
                relocs[0].howto = &reloc_howtos[RELOC_TYPE_ARM_MOV32];
            }
            relocs[0].offset = 0;
        } else if (ld_state->target_machine == LD_TARGET_MACHINE_AARCH64) {
            relocs[0].howto = &reloc_howtos[RELOC_TYPE_AARCH64_ADR_PREL_PG_HI21];
            relocs[0].offset = 0;
            relocs[1].symbol = &of->symbol_array[0];
            relocs[1].howto = &reloc_howtos[RELOC_TYPE_AARCH64_LDST64_ABS_LO12_NC];
            relocs[1].offset = 4;
        } else if (ld_state->target_machine == LD_TARGET_MACHINE_M68K) {
            relocs[0].howto = &reloc_howtos[RELOC_TYPE_32];
            relocs[0].offset = 2;
        } else if (ld_state->target_machine == LD_TARGET_MACHINE_MAINFRAME) {
            relocs[0].howto = &reloc_howtos[RELOC_TYPE_32];
            relocs[0].offset = 8;
        }
    }
    
    symbol->name = xmalloc (IMP_PREFIX_LEN + strlen (import_name) + 1);
    memcpy (symbol->name, IMP_PREFIX_STR, IMP_PREFIX_LEN);
    strcpy (symbol->name + IMP_PREFIX_LEN, import_name);
    
    symbol->value = 0;
    symbol->part = dot_idata5_part;
    symbol->section_number = 2;
    symbol_record_external_symbol (symbol);
}

static void import_generate_end (void)
{
    struct object_file *of;
    struct section *section;
    struct subsection *subsection;
    struct section_part *part;

    of = object_file_make (0, FAKE_LD_FILENAME);
    section = section_find_or_make (".idata");

    subsection = subsection_find_or_make (section, "4");
    part = section_part_new (section, of);
    subsection_append_section_part (subsection, part);
    part->content_size = (ld_state->target_machine == LD_TARGET_MACHINE_X64
                          || ld_state->target_machine == LD_TARGET_MACHINE_AARCH64) ? 8 : 4;
    part->content = xmalloc (part->content_size);
    memset (part->content, 0, part->content_size);

    subsection = subsection_find_or_make (section, "5");
    part = section_part_new (section, of);
    subsection_append_section_part (subsection, part);
    part->content_size = (ld_state->target_machine == LD_TARGET_MACHINE_X64
                          || ld_state->target_machine == LD_TARGET_MACHINE_AARCH64) ? 8 : 4;
    part->content = xmalloc (part->content_size);
    memset (part->content, 0, part->content_size);
}

static int read_import_object (unsigned char *file, size_t file_size, const char *filename)
{
    struct IMPORT_OBJECT_HEADER_internal import_hdr;
    char *import_name;
    const char *dll_name;
    
    unsigned char *pos;

    pos = file;
    CHECK_READ (pos, SIZEOF_struct_IMPORT_OBJECT_HEADER_file);
    read_struct_IMPORT_OBJECT_HEADER (&import_hdr, pos);
    pos += SIZEOF_struct_IMPORT_OBJECT_HEADER_file;

    if (check_Machine (import_hdr.Machine, filename)) return 1;

    if ((import_hdr.Type & 0x3) == IMPORT_CONST) {
        ld_internal_error_at_source (__FILE__, __LINE__,
                                     "+++not yet supported import header import Type: 0x%x",
                                     import_hdr.Type & 0x3);
    }
        
    CHECK_READ (pos, 2);
    import_name = (char *)pos;
    dll_name = import_name + strlen (import_name) + 1;

    if (ld_state->oformat == LD_OFORMAT_LX) {
        lx_import_generate_import_with_dll_name (import_name,
                                                 import_hdr.OrdinalHint,
                                                 import_hdr.Type & 0x3,
                                                 import_hdr.Type >> 2,
                                                 filename,
                                                 dll_name);
        return 0;
    }

    if (current_import_dll_name && strcmp (dll_name, current_import_dll_name)) {
        import_generate_end ();
        free (current_import_dll_name);
        current_import_dll_name = NULL;
    }
    if (current_import_dll_name == NULL) {
        current_import_dll_name = xstrdup (dll_name);
        import_generate_head (current_import_dll_name, filename);
    }
    
    import_generate_import (import_name,
                            import_hdr.OrdinalHint,
                            import_hdr.Type & 0x3,
                            import_hdr.Type >> 2,
                            NULL,
                            filename);

    return 0;
}

int coff_read (unsigned char *file, size_t file_size, const char *filename)
{
    unsigned short Machine;
    
    CHECK_READ (file, strlen (IMAGE_ARCHIVE_START));

    bytearray_read_2_bytes (&Machine, file, LITTLE_ENDIAN);

    if (Machine == IMAGE_FILE_MACHINE_AMD64
        || Machine == IMAGE_FILE_MACHINE_ARM
        || Machine == IMAGE_FILE_MACHINE_ARM64
        || Machine == IMAGE_FILE_MACHINE_ARMNT
        || Machine == IMAGE_FILE_MACHINE_I386
        || Machine == IMAGE_FILE_MACHINE_THUMB) {
        read_coff_object (file, file_size, filename);
        return INPUT_FILE_FINISHED;
    } else if (Machine == IMAGE_FILE_MACHINE_UNKNOWN) {
        unsigned short Magic2;
        
        CHECK_READ (file + 2, 2);
        bytearray_read_2_bytes (&Magic2, file + 2, LITTLE_ENDIAN);
        if (Magic2 == IMPORT_OBJECT_HDR_MAGIC2) {
            if (read_import_object (file, file_size, filename)) {
                return INPUT_FILE_ERROR;
            } else {
                return INPUT_FILE_NO_NEW_SYMBOLS;
            }
        } else {
            return INPUT_FILE_UNRECOGNIZED;
        }
    } else {
        return INPUT_FILE_UNRECOGNIZED;
    }

    return INPUT_FILE_FINISHED;
}

int coff_is_import_header (unsigned char *file, size_t file_size)
{
    unsigned short Machine;

    if (file_size < 4) return 0;
    bytearray_read_2_bytes (&Machine, file, LITTLE_ENDIAN);
    if (Machine == IMAGE_FILE_MACHINE_UNKNOWN) {
        unsigned short Magic2;
        
        bytearray_read_2_bytes (&Magic2, file + 2, LITTLE_ENDIAN);
        if (Magic2 == IMPORT_OBJECT_HDR_MAGIC2) return 1;
    }

    return 0;
}

void coff_archive_end (void)
{
    if (current_import_dll_name) {
        import_generate_end ();
        free (current_import_dll_name);
        current_import_dll_name = NULL;
    }
}

void coff_direct_import (const char *internal_name,
                         const char *dll_name,
                         const char *real_import_name,
                         const char *filename)
{
    if (ld_state->oformat == LD_OFORMAT_LX) {
        ld_internal_error_at_source (__FILE__, __LINE__,
                                     "%s: coff_direct_import () (OMF import) to LX is not yet supported",
                                     filename);
    }

    if (current_import_dll_name && strcmp (dll_name, current_import_dll_name)) {
        import_generate_end ();
        free (current_import_dll_name);
        current_import_dll_name = NULL;
    }
    if (current_import_dll_name == NULL) {
        current_import_dll_name = xstrdup (dll_name);
        import_generate_head (current_import_dll_name, filename);
    }
    
    import_generate_import (internal_name, 0, IMPORT_CODE, IMPORT_NAME, real_import_name, filename);
}

#include "options.h"

enum option_index {

    COFF_OPTION_IGNORED = 0,
    COFF_OPTION_FILE_ALIGNMENT,
    COFF_OPTION_SECTION_ALIGNMENT,
    COFF_OPTION_STACK,
    COFF_OPTION_STUB,
    COFF_OPTION_SUBSYSTEM,
    COFF_OPTION_INSERT_TIMESTAMP,
    COFF_OPTION_NO_INSERT_TIMESTAMP,
    COFF_OPTION_EXPORT_ALL_SYMBOLS,
    COFF_OPTION_KILL_AT,
    COFF_OPTION_ENABLE_RELOC_SECTION,
    COFF_OPTION_DISABLE_RELOC_SECTION,
    COFF_OPTION_NX_COMPAT,
    COFF_OPTION_DISABLE_NX_COMPAT,
    COFF_OPTION_IMPLIB_COMPAT

};

#define STR_AND_LEN(str) (str), (sizeof (str) - 1)
static const struct long_option long_options[] = {
    
    { STR_AND_LEN("file-alignment"), COFF_OPTION_FILE_ALIGNMENT, OPTION_HAS_ARG},
    { STR_AND_LEN("section-alignment"), COFF_OPTION_SECTION_ALIGNMENT, OPTION_HAS_ARG},
    { STR_AND_LEN("stack"), COFF_OPTION_STACK, OPTION_HAS_ARG},
    { STR_AND_LEN("stub"), COFF_OPTION_STUB, OPTION_HAS_ARG},
    { STR_AND_LEN("subsystem"), COFF_OPTION_SUBSYSTEM, OPTION_HAS_ARG},
    { STR_AND_LEN("insert-timestamp"), COFF_OPTION_INSERT_TIMESTAMP, OPTION_NO_ARG},
    { STR_AND_LEN("no-insert-timestamp"), COFF_OPTION_NO_INSERT_TIMESTAMP, OPTION_NO_ARG},
    { STR_AND_LEN("export-all-symbols"), COFF_OPTION_EXPORT_ALL_SYMBOLS, OPTION_NO_ARG},
    { STR_AND_LEN("kill-at"), COFF_OPTION_KILL_AT, OPTION_NO_ARG},
    { STR_AND_LEN("enable-reloc-section"), COFF_OPTION_ENABLE_RELOC_SECTION, OPTION_NO_ARG},
    { STR_AND_LEN("disable-reloc-section"), COFF_OPTION_DISABLE_RELOC_SECTION, OPTION_NO_ARG},
    { STR_AND_LEN("nxcompat"), COFF_OPTION_NX_COMPAT, OPTION_NO_ARG},
    { STR_AND_LEN("disable-nxcompat"), COFF_OPTION_DISABLE_NX_COMPAT, OPTION_NO_ARG},
    { STR_AND_LEN("implib-compat"), COFF_OPTION_IMPLIB_COMPAT, OPTION_NO_ARG},
    { NULL, 0, 0}

};
#undef STR_AND_LEN

void coff_print_help (void)
{
    printf ("i386pe:\n");
    printf ("  --file-alignment <size>            Set file alignment\n");
    printf ("  --section-alignment <size>         Set section alignment\n");
    printf ("  --stack <reserve size>[,<commit size>]\n"
            "                                     Set size of the initial stack\n");
    printf ("  --stub FILE                        Use FILE as MZ stub\n");
    printf ("  --subsystem <name>[:<version>]     Set required OS subsystem [& version]\n");
    printf ("  --[no-]insert-timestamp            Use a real timestamp (default) rather than zero.\n");
    printf ("                                     This makes binaries non-deterministic\n");
    printf ("  --export-all-symbols               Automatically export all globals to DLL\n");
    printf ("  --kill-at                          Remove @nn from exported symbols\n");
    printf ("  --enable-reloc-section             Create the base relocation table\n");
    printf ("  --disable-reloc-section            Do not create the base relocation table\n");
    printf ("  --[disable-]nxcompat               Image is compatible with data execution\n");
    printf ("                                       prevention\n");
    printf ("  --implib-compat                    --out-implib generates extra archive members\n");
    printf ("                                       for compatibility with other linkers\n");
}

static void use_option (enum option_index option_index, char *arg)
{
    switch (option_index) {

        case COFF_OPTION_IGNORED:
            break;

        case COFF_OPTION_FILE_ALIGNMENT:
            {
                char *p;
                
                FileAlignment = strtoul (arg, &p, 0);
                if (FileAlignment == 0) FileAlignment = 1;
                if (*p != '\0') {
                    ld_error ("invalid file alignment number '%s'", arg);
                    break;
                }

                if (FileAlignment < 512 || FileAlignment > 0x10000 || (FileAlignment & (FileAlignment - 1))) {
                    ld_warn ("file alignment should be a power of two between 512 and 64 KiB (0x10000) inclusive according to the specification");
                }
            }
            break;

        case COFF_OPTION_SECTION_ALIGNMENT:
            {
                char *p;
                
                SectionAlignment = strtoul (arg, &p, 0);
                if (SectionAlignment == 0) SectionAlignment = 1;
                if (*p != '\0') {
                    ld_error ("invalid section alignment number '%s'", arg);
                    break;
                }

                if (SectionAlignment < FileAlignment) {
                    ld_warn ("section alignment must be greater than or equal to file alignment according to the specification");
                }
            }
            break;

        case COFF_OPTION_STACK:
            {
                char *p;

                if (*arg != ',') {
                    SizeOfStackReserve = strtoul (arg, &p, 0);
                    if (*p == 0) break;
                    if (*p != ',') {
                        ld_error ("invalid stack reserve size number '%s'", arg);
                        break;
                    }
                    arg = p;
                }
                
                arg++;
                SizeOfStackCommit = strtoul (arg, &p, 0);
                if (*p != '\0') {
                    ld_error ("invalid stack commit size number '%s'", arg);
                    break;
                }
            }
            break;

        case COFF_OPTION_STUB:
            {
                struct IMAGE_DOS_HEADER_internal dos_hdr;
                size_t old_hdr_size, new_hdr_size;
                
                if (read_file_into_memory (arg, &stub_file, &stub_size)) {
                    ld_error ("failed to read stub file '%s' into memory", arg);
                    return;
                }

                if (stub_size < SIZEOF_struct_IMAGE_DOS_HEADER_file
                    || !(stub_file[0] == 'M' && stub_file[1] == 'Z')) {
                    free (stub_file);
                    stub_file = NULL;
                    ld_error ("'%s' is not a valid MZ executable", arg);
                    return;
                }

                /* It is possible that the provided stub has smaller header
                 * than needed for OffsetToNewEXEHeader field,
                 * so the header needs to be expanded and the stub adjusted. */
                read_struct_IMAGE_DOS_HEADER (&dos_hdr, stub_file);
                old_hdr_size = dos_hdr.SizeOfHeaderInParagraphs * IMAGE_DOS_HEADER_PARAGRAPH_SIZE;
                new_hdr_size = SIZEOF_struct_IMAGE_DOS_HEADER_file;
                if (old_hdr_size < new_hdr_size) {
                    unsigned char *new_stub;
                    
                    stub_size += new_hdr_size - old_hdr_size;
                    new_stub = xmalloc (stub_size);
                    memcpy (new_stub + new_hdr_size,
                            stub_file + old_hdr_size,
                            stub_size - new_hdr_size);

                    dos_hdr.SizeOfHeaderInParagraphs = new_hdr_size / IMAGE_DOS_HEADER_PARAGRAPH_SIZE;
                    dos_hdr.OffsetToRelocationTable += new_hdr_size - old_hdr_size;

                    write_struct_IMAGE_DOS_HEADER (new_stub, &dos_hdr);
                    memset (new_stub + old_hdr_size,
                            '\0',
                            new_hdr_size - old_hdr_size);
                    
                    free (stub_file);
                    stub_file = new_stub;
                }
            }
            break;

        case COFF_OPTION_SUBSYSTEM:
            {
                char *p;

                Subsystem = strtoul (arg, &p, 0);
                if (*p == '\0') break;
                if (*p != ':') goto bad_subsystem;

                p++;
                MajorSubsystemVersion = strtoul (p, &p, 0);
                if (*p == '\0') break;
                if (*p != '.') goto bad_subsystem;

                p++;
                MinorSubsystemVersion = strtoul (p, &p, 0);
                if (*p == '\0') break;

            bad_subsystem:
                ld_error ("invalid subsystem type '%s'", arg);
            }
            break;

        case COFF_OPTION_INSERT_TIMESTAMP:
            insert_timestamp = 1;
            break;

        case COFF_OPTION_NO_INSERT_TIMESTAMP:
            insert_timestamp = 0;
            break;

        case COFF_OPTION_EXPORT_ALL_SYMBOLS:
            export_all_symbols = 1;
            break;

        case COFF_OPTION_KILL_AT:
            kill_at = 1;
            break;

        case COFF_OPTION_ENABLE_RELOC_SECTION:
            generate_reloc_section = 1;
            break;

        case COFF_OPTION_DISABLE_RELOC_SECTION:
            generate_reloc_section = 0;
            break;

        case COFF_OPTION_NX_COMPAT:
            nx_compat = 1;
            break;

        case COFF_OPTION_DISABLE_NX_COMPAT:
            nx_compat = 0;
            break;

        case COFF_OPTION_IMPLIB_COMPAT:
            implib_compat = 1;
            break;

    }
}

void coff_use_option (int option_index, char *arg)
{
    use_option (option_index, arg);
}

const struct long_option *coff_get_long_options (void)
{
    return long_options;
}
