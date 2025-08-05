/******************************************************************************
 * @file            coff_bytearray.h
 *
 * Released to the public domain.
 *
 * Anyone and anything may copy, edit, publish, use, compile, sell and
 * distribute this work and all its parts in any form for any purpose,
 * commercial and non-commercial, without any restrictions, without
 * complying with any conditions and by any means.
 *****************************************************************************/ 

#define read_struct_IMAGE_DOS_HEADER rsidh
#define read_struct_coff_header rsch
#define read_struct_optional_header rsoh
#define read_struct_optional_header_plus rsohp
#define read_struct_IMAGE_DATA_DIRECTORY rsidd
#define read_struct_section_table_entry rssete
#define read_struct_relocation_entry rsre
#define read_struct_symbol_table_entry rssyte
#define read_struct_aux_section_symbol rsass
#define read_struct_string_table_header rssth
#define read_struct_IMAGE_BASE_RELOCATION rsibr
#define read_struct_IMAGE_ARCHIVE_MEMBER_HEADER rsiamh
#define read_struct_IMPORT_OBJECT_HEADER rsioh

#define write_struct_IMAGE_DOS_HEADER wsidh
#define write_struct_coff_header wsch
#define write_struct_optional_header wsoh
#define write_struct_optional_header_plus wsohp
#define write_struct_IMAGE_DATA_DIRECTORY wsidd
#define write_struct_section_table_entry wsste
#define write_struct_IMAGE_EXPORT_DIRECTORY wsied
#define write_struct_EXPORT_Name_Pointer_Table wsenpt
#define write_struct_EXPORT_Ordinal_Table wseot
#define write_struct_IMPORT_Directory_Table wsidt
#define write_struct_IMAGE_BASE_RELOCATION wsibr
#define write_struct_IMAGE_ARCHIVE_MEMBER_HEADER wsiamh
#define write_struct_IMPORT_OBJECT_HEADER wsioh
#define write_struct_symbol_table_entry wssymte
#define write_struct_string_table_header wssth

void read_struct_IMAGE_DOS_HEADER (struct IMAGE_DOS_HEADER_internal *IMAGE_DOS_HEADER_internal, const void *memory);
void read_struct_coff_header (struct coff_header_internal *coff_header_internal, const void *memory);
void read_struct_optional_header (struct optional_header_internal *optional_header_internal, const void *memory);
void read_struct_optional_header_plus (struct optional_header_plus_internal *optional_header_plus_internal, const void *memory);
void read_struct_IMAGE_DATA_DIRECTORY (struct IMAGE_DATA_DIRECTORY_internal *IMAGE_DATA_DIRECTORY_internal, const void *memory);
void read_struct_section_table_entry (struct section_table_entry_internal *section_table_entry_internal, const void *memory);
void read_struct_relocation_entry (struct relocation_entry_internal *relocation_entry_internal, const void *memory);
void read_struct_symbol_table_entry (struct symbol_table_entry_internal *symbol_table_entry_internal, const void *memory);
void read_struct_aux_section_symbol (struct aux_section_symbol_internal *aux_section_symbol_internal, const void *memory);
void read_struct_string_table_header (struct string_table_header_internal *string_table_header_internal, const void *memory);
void read_struct_IMAGE_BASE_RELOCATION (struct IMAGE_BASE_RELOCATION_internal *IMAGE_BASE_RELOCATION_internal, const void *memory);
void read_struct_IMAGE_ARCHIVE_MEMBER_HEADER (struct IMAGE_ARCHIVE_MEMBER_HEADER_internal *IMAGE_ARCHIVE_MEMBER_HEADER_internal, const void *memory);
void read_struct_IMPORT_OBJECT_HEADER (struct IMPORT_OBJECT_HEADER_internal *IMPORT_OBJECT_HEADER_internal, const void *memory);

void write_struct_IMAGE_DOS_HEADER (void *memory, const struct IMAGE_DOS_HEADER_internal *IMAGE_DOS_HEADER_internal);
void write_struct_coff_header (void *memory, const struct coff_header_internal *coff_header_internal);
void write_struct_optional_header (void *memory, const struct optional_header_internal *optional_header_internal);
void write_struct_optional_header_plus (void *memory, const struct optional_header_plus_internal *optional_header_plus_internal);
void write_struct_IMAGE_DATA_DIRECTORY (void *memory, const struct IMAGE_DATA_DIRECTORY_internal *IMAGE_DATA_DIRECTORY_internal);
void write_struct_section_table_entry (void *memory, const struct section_table_entry_internal *section_table_entry_internal);
void write_struct_relocation_entry (void *memory, const struct relocation_entry_internal *relocation_entry_internal);
void write_struct_symbol_table_entry (void *memory, const struct symbol_table_entry_internal *symbol_table_entry_internal);
void write_struct_string_table_header (void *memory, const struct string_table_header_internal *string_table_header_internal);
void write_struct_IMAGE_EXPORT_DIRECTORY (void *memory, const struct IMAGE_EXPORT_DIRECTORY_internal *IMAGE_EXPORT_DIRECTORY_internal);
void write_struct_EXPORT_Name_Pointer_Table (void *memory, const struct EXPORT_Name_Pointer_Table_internal *EXPORT_Name_Pointer_Table_internal);
void write_struct_EXPORT_Ordinal_Table (void *memory, const struct EXPORT_Ordinal_Table_internal *EXPORT_Ordinal_Table_internal);
void write_struct_IMPORT_Directory_Table (void *memory, const struct IMPORT_Directory_Table_internal *IMPORT_Directory_Table_internal);
void write_struct_IMAGE_BASE_RELOCATION (void *memory, const struct IMAGE_BASE_RELOCATION_internal *IMAGE_BASE_RELOCATION_internal);
void write_struct_IMAGE_ARCHIVE_MEMBER_HEADER (void *memory, const struct IMAGE_ARCHIVE_MEMBER_HEADER_internal *IMAGE_ARCHIVE_MEMBER_HEADER_internal);
void write_struct_IMPORT_OBJECT_HEADER (void *memory, const struct IMPORT_OBJECT_HEADER_internal *IMPORT_OBJECT_HEADER_internal);
