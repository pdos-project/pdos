/******************************************************************************
 * @file            coff.h
 *
 * Released to the public domain.
 *
 * Anyone and anything may copy, edit, publish, use, compile, sell and
 * distribute this work and all its parts in any form for any purpose,
 * commercial and non-commercial, without any restrictions, without
 * complying with any conditions and by any means.
 *****************************************************************************/
#define IMAGE_DOS_HEADER_PARAGRAPH_SIZE 16

struct IMAGE_DOS_HEADER_internal {

    unsigned char Magic[2]; /* "MZ" */
    unsigned short dummy1;
    unsigned short dummy2;
    unsigned short dummy3;
    unsigned short SizeOfHeaderInParagraphs;
    unsigned short MinimumExtraParagraphs;
    unsigned short MaximumExtraParagraphs;
    unsigned short InitialSs;
    unsigned short InitialSp;
    unsigned short Checksum;
    unsigned short InitialIp;
    unsigned short InitialCs;
    unsigned short OffsetToRelocationTable;
    unsigned short OverlayNumber;
    unsigned char dummy4[32];
    unsigned long OffsetToNewEXEHeader;

};

#define SIZEOF_struct_IMAGE_DOS_HEADER_file 64
struct IMAGE_DOS_HEADER_file {

    unsigned char Magic[2];
    unsigned char dummy1[2];
    unsigned char dummy2[2];
    unsigned char dummy3[2];
    unsigned char SizeOfHeaderInParagraphs[2];
    unsigned char MinimumExtraParagraphs[2];
    unsigned char MaximumExtraParagraphs[2];
    unsigned char InitialSs[2];
    unsigned char InitialSp[2];
    unsigned char Checksum[2];
    unsigned char InitialIp[2];
    unsigned char InitialCs[2];
    unsigned char OffsetToRelocationTable[2];
    unsigned char OverlayNumber[2];
    unsigned char dummy4[32];
    unsigned char OffsetToNewEXEHeader[4];

};
    
struct coff_header_internal {

    unsigned short Machine;
    unsigned short NumberOfSections;
    
    unsigned long TimeDateStamp;
    unsigned long PointerToSymbolTable;
    unsigned long NumberOfSymbols;
    
    unsigned short SizeOfOptionalHeader;
    unsigned short Characteristics;

};

#define SIZEOF_struct_coff_header_file 20
struct coff_header_file {

    unsigned char Machine[2];
    unsigned char NumberOfSections[2];
    
    unsigned char TimeDateStamp[4];
    unsigned char PointerToSymbolTable[4];
    unsigned char NumberOfSymbols[4];
    
    unsigned char SizeOfOptionalHeader[2];
    unsigned char Characteristics[2];

};

#define     IMAGE_FILE_MACHINE_UNKNOWN                      0x0
#define     IMAGE_FILE_MACHINE_AMD64                        0x8664
#define     IMAGE_FILE_MACHINE_ARM                          0x1c0
#define     IMAGE_FILE_MACHINE_ARM64                        0xaa64
#define     IMAGE_FILE_MACHINE_ARMNT                        0x1c4
#define     IMAGE_FILE_MACHINE_I386                         0x14c
#define     IMAGE_FILE_MACHINE_THUMB                        0x1c2
#define     IMAGE_FILE_MACHINE_M68K                         0x268

#define     IMAGE_FILE_RELOCS_STRIPPED                      0x0001
#define     IMAGE_FILE_EXECUTABLE_IMAGE                     0x0002
#define     IMAGE_FILE_LINE_NUMS_STRIPPED                   0x0004 /* Deprecated, should be 0. */
#define     IMAGE_FILE_LOCAL_SYMS_STRIPPED                  0x0008 /* Deprecated, should be 0. */
#define     IMAGE_FILE_LARGE_ADDRESS_AWARE                  0x0020
#define     IMAGE_FILE_32BIT_MACHINE                        0x0100
#define     IMAGE_FILE_DEBUG_STRIPPED                       0x0200
#define     IMAGE_FILE_DLL                                  0x2000

struct optional_header_internal {

    unsigned short Magic;
    unsigned char MajorLinkerVersion;
    unsigned char MinorLinkerVersion;
    unsigned long SizeOfCode;
    unsigned long SizeOfInitializedData;
    unsigned long SizeOfUninitializedData;
    unsigned long AddressOfEntryPoint;
    unsigned long BaseOfCode;
    unsigned long BaseOfData;

#define DEFAULT_EXE_IMAGE_BASE 0x00400000
#define DEFAULT_DLL_IMAGE_BASE 0x10000000
#if !(defined (NO_LONG_LONG) && ((ULONG_MAX >> 16) >> 16) < 0xffffffff)
#define DEFAULT_ARM64_IMAGE_BASE 0x140000000
#else
#define DEFAULT_ARM64_IMAGE_BASE 0
#endif
    unsigned long ImageBase;
#define DEFAULT_SECTION_ALIGNMENT 0x1000
    unsigned long SectionAlignment;
#define DEFAULT_FILE_ALIGNMENT 512
    unsigned long FileAlignment;
    unsigned short MajorOperatingSystemVersion;
    unsigned short MinorOperatingSystemVersion;
    unsigned short MajorImageVersion;
    unsigned short MinorImageVersion;
    unsigned short MajorSubsystemVersion;
    unsigned short MinorSubsystemVersion;
    unsigned long Win32VersionValue; /* Reserved, must be 0. */
    unsigned long SizeOfImage; /* Must be multiple of SectionAlignment. */
    unsigned long SizeOfHeaders; /* Rounded up to FileAlignment. */
    unsigned long CheckSum;
    unsigned short Subsystem;
    unsigned short DllCharacteristics;
    unsigned long SizeOfStackReserve;
    unsigned long SizeOfStackCommit;
    unsigned long SizeOfHeapReserve;
    unsigned long SizeOfHeapCommit;
    unsigned long LoaderFlags; /* Reserved, must be 0. */
    unsigned long NumberOfRvaAndSizes;

};

#define SIZEOF_struct_optional_header_file 96
struct optional_header_file {

    unsigned char Magic[2];
    unsigned char MajorLinkerVersion[1];
    unsigned char MinorLinkerVersion[1];
    unsigned char SizeOfCode[4];
    unsigned char SizeOfInitializedData[4];
    unsigned char SizeOfUninitializedData[4];
    unsigned char AddressOfEntryPoint[4];
    unsigned char BaseOfCode[4];
    unsigned char BaseOfData[4];

    unsigned char ImageBase[4];
    unsigned char SectionAlignment[4];
    unsigned char FileAlignment[4];
    unsigned char MajorOperatingSystemVersion[2];
    unsigned char MinorOperatingSystemVersion[2];
    unsigned char MajorImageVersion[2];
    unsigned char MinorImageVersion[2];
    unsigned char MajorSubsystemVersion[2];
    unsigned char MinorSubsystemVersion[2];
    unsigned char Win32VersionValue[4];
    unsigned char SizeOfImage[4];
    unsigned char SizeOfHeaders[4];
    unsigned char CheckSum[4];
    unsigned char Subsystem[2];
    unsigned char DllCharacteristics[2];
    unsigned char SizeOfStackReserve[4];
    unsigned char SizeOfStackCommit[4];
    unsigned char SizeOfHeapReserve[4];
    unsigned char SizeOfHeapCommit[4];
    unsigned char LoaderFlags[4];
    unsigned char NumberOfRvaAndSizes[4];

};

struct optional_header_plus_internal {

    unsigned short Magic;
    unsigned char MajorLinkerVersion;
    unsigned char MinorLinkerVersion;
    unsigned long SizeOfCode;
    unsigned long SizeOfInitializedData;
    unsigned long SizeOfUninitializedData;
    unsigned long AddressOfEntryPoint;
    unsigned long BaseOfCode;

    uint_fast64_t ImageBase;
    
    unsigned long SectionAlignment;
    unsigned long FileAlignment;
    unsigned short MajorOperatingSystemVersion;
    unsigned short MinorOperatingSystemVersion;
    unsigned short MajorImageVersion;
    unsigned short MinorImageVersion;
    unsigned short MajorSubsystemVersion;
    unsigned short MinorSubsystemVersion;
    unsigned long Win32VersionValue; /* Reserved, must be 0. */
    unsigned long SizeOfImage; /* Must be multiple of SectionAlignment. */
    unsigned long SizeOfHeaders; /* Rounded up to FileAlignment. */
    unsigned long CheckSum;
    unsigned short Subsystem;
    unsigned short DllCharacteristics;
    
    uint_fast64_t SizeOfStackReserve;
    uint_fast64_t SizeOfStackCommit;
    uint_fast64_t SizeOfHeapReserve;
    uint_fast64_t SizeOfHeapCommit;
    
    unsigned long LoaderFlags; /* Reserved, must be 0. */
    unsigned long NumberOfRvaAndSizes;

};

#define SIZEOF_struct_optional_header_plus_file 112
struct optional_header_plus_file {

    unsigned char Magic[2];
    unsigned char MajorLinkerVersion[1];
    unsigned char MinorLinkerVersion[1];
    unsigned char SizeOfCode[4];
    unsigned char SizeOfInitializedData[4];
    unsigned char SizeOfUninitializedData[4];
    unsigned char AddressOfEntryPoint[4];
    unsigned char BaseOfCode[4];

    unsigned char ImageBase[8];
    
    unsigned char SectionAlignment[4];
    unsigned char FileAlignment[4];
    unsigned char MajorOperatingSystemVersion[2];
    unsigned char MinorOperatingSystemVersion[2];
    unsigned char MajorImageVersion[2];
    unsigned char MinorImageVersion[2];
    unsigned char MajorSubsystemVersion[2];
    unsigned char MinorSubsystemVersion[2];
    unsigned char Win32VersionValue[4];
    unsigned char SizeOfImage[4];
    unsigned char SizeOfHeaders[4];
    unsigned char CheckSum[4];
    unsigned char Subsystem[2];
    unsigned char DllCharacteristics[2];
    
    unsigned char SizeOfStackReserve[8];
    unsigned char SizeOfStackCommit[8];
    unsigned char SizeOfHeapReserve[8];
    unsigned char SizeOfHeapCommit[8];
    
    unsigned char LoaderFlags[4];
    unsigned char NumberOfRvaAndSizes[4];

};

#define IMAGE_NT_OPTIONAL_HDR32_MAGIC 0x10b /* PE32 */
#define PE32_PLUS_MAGIC 0x20b

#define IMAGE_SUBSYSTEM_UNKNOWN 0
#define IMAGE_SUBSYSTEM_NATIVE 1
#define IMAGE_SUBSYSTEM_WINDOWS_GUI 2
#define IMAGE_SUBSYSTEM_WINDOWS_CUI 3
#define IMAGE_SUBSYSTEM_OS2_CUI 5
#define IMAGE_SUBSYSTEM_POSIX_CUI 7
#define IMAGE_SUBSYSTEM_EFI_APPLICATION 10
#define IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER 11
#define IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER 12
#define IMAGE_SUBSYSTEM_EFI_ROM 13
#define IMAGE_SUBSYSTEM_XBOX 14
#define IMAGE_SUBSYSTEM_WINDOWS_BOOT_APPILCATION 16

#define IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE 0x0040
#define IMAGE_DLLCHARACTERISTICS_NX_COMPAT 0x0100

struct IMAGE_DATA_DIRECTORY_internal {
    
    unsigned long VirtualAddress;
    unsigned long Size;
    
};

#define SIZEOF_struct_IMAGE_DATA_DIRECTORY_file 8
struct IMAGE_DATA_DIRECTORY_file {
    
    unsigned char VirtualAddress[4];
    unsigned char Size[4];
    
};

struct section_table_entry_internal {

    char Name[8];
    
    unsigned long VirtualSize;
    unsigned long VirtualAddress;
    
    unsigned long SizeOfRawData;
    unsigned long PointerToRawData;
    unsigned long PointerToRelocations;
    unsigned long PointerToLinenumbers;
    
    unsigned short NumberOfRelocations;
    unsigned short NumberOfLinenumbers;
    
    unsigned long Characteristics;

};

#define SIZEOF_struct_section_table_entry_file 40
struct section_table_entry_file {

    char Name[8];
    
    unsigned char VirtualSize[4];
    unsigned char VirtualAddress[4];
    
    unsigned char SizeOfRawData[4];
    unsigned char PointerToRawData[4];
    unsigned char PointerToRelocations[4];
    unsigned char PointerToLinenumbers[4];
    
    unsigned char NumberOfRelocations[2];
    unsigned char NumberOfLinenumbers[2];
    
    unsigned char Characteristics[4];

};

#define     IMAGE_SCN_TYPE_NOLOAD                           0x00000002
#define     IMAGE_SCN_TYPE_NO_PAD                           0x00000008 /* Obsolete, means the same as IMAGE_SCN_ALIGN_1BYTES. */
#define     IMAGE_SCN_CNT_CODE                              0x00000020
#define     IMAGE_SCN_CNT_INITIALIZED_DATA                  0x00000040
#define     IMAGE_SCN_CNT_UNINITIALIZED_DATA                0x00000080
#define     IMAGE_SCN_LNK_INFO                              0x00000200
#define     IMAGE_SCN_LNK_REMOVE                            0x00000800
#define     IMAGE_SCN_LNK_COMDAT                            0x00001000
#define     IMAGE_SCN_ALIGN_1BYTES                          0x00100000
#define     IMAGE_SCN_ALIGN_2BYTES                          0x00200000
#define     IMAGE_SCN_ALIGN_4BYTES                          0x00300000
#define     IMAGE_SCN_ALIGN_8BYTES                          0x00400000
#define     IMAGE_SCN_ALIGN_16BYTES                         0x00500000
#define     IMAGE_SCN_ALIGN_32BYTES                         0x00600000
#define     IMAGE_SCN_ALIGN_64BYTES                         0x00700000
#define     IMAGE_SCN_ALIGN_128BYTES                        0x00800000
#define     IMAGE_SCN_ALIGN_256BYTES                        0x00900000
#define     IMAGE_SCN_ALIGN_512BYTES                        0x00A00000
#define     IMAGE_SCN_ALIGN_1024BYTES                       0x00B00000
#define     IMAGE_SCN_ALIGN_2048BYTES                       0x00C00000
#define     IMAGE_SCN_ALIGN_4096BYTES                       0x00D00000
#define     IMAGE_SCN_ALIGN_8192BYTES                       0x00E00000
#define     IMAGE_SCN_MEM_SHARED                            0x10000000
#define     IMAGE_SCN_MEM_EXECUTE                           0x20000000
#define     IMAGE_SCN_MEM_READ                              0x40000000
#define     IMAGE_SCN_MEM_WRITE                             0x80000000

struct relocation_entry_internal {

    unsigned long VirtualAddress;
    unsigned long SymbolTableIndex;
    
    unsigned short Type;

};

#define SIZEOF_struct_relocation_entry_file 10
struct relocation_entry_file {

    unsigned char VirtualAddress[4];
    unsigned char SymbolTableIndex[4];
    
    unsigned char Type[2];

};

#define     IMAGE_REL_AMD64_ABSOLUTE                        0x0000
#define     IMAGE_REL_AMD64_ADDR64                          0x0001
#define     IMAGE_REL_AMD64_ADDR32                          0x0002
#define     IMAGE_REL_AMD64_ADDR32NB                        0x0003
#define     IMAGE_REL_AMD64_REL32                           0x0004
#define     IMAGE_REL_AMD64_REL32_1                         0x0005
#define     IMAGE_REL_AMD64_REL32_2                         0x0006
#define     IMAGE_REL_AMD64_REL32_3                         0x0007
#define     IMAGE_REL_AMD64_REL32_4                         0x0008
#define     IMAGE_REL_AMD64_REL32_5                         0x0009
#define     IMAGE_REL_AMD64_SECTION                         0x000A
#define     IMAGE_REL_AMD64_SECREL                          0x000B
#define     IMAGE_REL_AMD64_SECREL7                         0x000C
#define     IMAGE_REL_AMD64_TOKEN                           0x000D
#define     IMAGE_REL_AMD64_SREL32                          0x000E
#define     IMAGE_REL_AMD64_PAIR                            0x000F
#define     IMAGE_REL_AMD64_SSPAN32                         0x0010

#define     IMAGE_REL_ARM_ABSOLUTE                          0x0000
#define     IMAGE_REL_ARM_ADDR32                            0x0001
#define     IMAGE_REL_ARM_ADDR32NB                          0x0002
#define     IMAGE_REL_ARM_BRANCH24                          0x0003
#define     IMAGE_REL_ARM_BRANCH11                          0x0004
#define     IMAGE_REL_ARM_SECTION                           0x000E
#define     IMAGE_REL_ARM_SECREL                            0x000F
#define     IMAGE_REL_ARM_MOV32                             0x0010
#define     IMAGE_REL_THUMB_MOV32                           0x0011
#define     IMAGE_REL_THUMB_BRANCH20                        0x0012
#define     IMAGE_REL_THUMB_BRANCH24                        0x0014
#define     IMAGE_REL_THUMB_BLX23                           0x0015
#define     IMAGE_REL_ARM_PAIR                              0x0016

#define     IMAGE_REL_ARM64_ABSOLUTE                        0x0000
#define     IMAGE_REL_ARM64_ADDR32                          0x0001
#define     IMAGE_REL_ARM64_ADDR32NB                        0x0002
#define     IMAGE_REL_ARM64_BRANCH26                        0x0003
#define     IMAGE_REL_ARM64_PAGEBASE_REL21                  0x0004
#define     IMAGE_REL_ARM64_REL21                           0x0005
#define     IMAGE_REL_ARM64_PAGEOFFSET_12A                  0x0006
#define     IMAGE_REL_ARM64_PAGEOFFSET_12L                  0x0007
#define     IMAGE_REL_ARM64_SECREL                          0x0008
#define     IMAGE_REL_ARM64_SECREL_LOW12A                   0x0009
#define     IMAGE_REL_ARM64_SECREL_HIGH12A                  0x000A
#define     IMAGE_REL_ARM64_SECREL_LOW12L                   0x000B
#define     IMAGE_REL_ARM64_TOKEN                           0x000C
#define     IMAGE_REL_ARM64_SECTION                         0x000D
#define     IMAGE_REL_ARM64_ADDR64                          0x000E
#define     IMAGE_REL_ARM64_BRANCH19                        0x000F
#define     IMAGE_REL_ARM64_BRANCH14                        0x0010

#define     IMAGE_REL_I386_ABSOLUTE                         0x0000
#define     IMAGE_REL_I386_DIR16                            0x0001 /* No longer supported. */
#define     IMAGE_REL_I386_REL16                            0x0002 /* No longer supported. */
#define     IMAGE_REL_I386_DIR32                            0x0006
#define     IMAGE_REL_I386_DIR32NB                          0x0007
#define     IMAGE_REL_I386_SEG12                            0x0009 /* No longer supported. */
#define     IMAGE_REL_I386_SECTION                          0x000A
#define     IMAGE_REL_I386_SECREL                           0x000B
#define     IMAGE_REL_I386_TOKEN                            0x000C
#define     IMAGE_REL_I386_SECREL7                          0x000D
#define     IMAGE_REL_I386_REL32                            0x0014

struct symbol_table_entry_internal {

    char Name[8];
    unsigned long Value;
    
    signed short SectionNumber;
    unsigned short Type;
    
    unsigned char StorageClass;
    unsigned char NumberOfAuxSymbols;

};

#define SIZEOF_struct_symbol_table_entry_file 18
struct symbol_table_entry_file {

    char Name[8];
    unsigned char Value[4];
    
    unsigned char SectionNumber[2];
    unsigned char Type[2];
    
    unsigned char StorageClass[1];
    unsigned char NumberOfAuxSymbols[1];

};

#define     IMAGE_SYM_UNDEFINED                             0
#define     IMAGE_SYM_ABSOLUTE                              -1
#define     IMAGE_SYM_DEBUG                                 -2

#define     IMAGE_SYM_TYPE_NULL                             0
#define     IMAGE_SYM_DTYPE_NULL                            0

#define     IMAGE_SYM_CLASS_EXTERNAL                        2
#define     IMAGE_SYM_CLASS_STATIC                          3
#define     IMAGE_SYM_CLASS_LABEL                           6
#define     IMAGE_SYM_CLASS_FILE                            103
#define     IMAGE_SYM_CLASS_SECTION                         104

struct aux_section_symbol_internal {

    unsigned long Length;
    unsigned short NumberOfRelocations;
    unsigned short NumberOfLinenumbers;
    
    unsigned long CheckSum;
    unsigned short Number;
    unsigned char Selection;
    
    unsigned char Unused[3];

};

#define SIZEOF_struct_aux_section_symbol_file 18
struct aux_section_symbol_file {

    unsigned char Length[4];
    unsigned char NumberOfRelocations[2];
    unsigned char NumberOfLinenumbers[2];
    
    unsigned char CheckSum[4];
    unsigned char Number[2];
    unsigned char Selection[1];
    
    unsigned char Unused[3];

};

#define IMAGE_COMDAT_SELECT_NODUPLICATES 1
#define IMAGE_COMDAT_SELECT_ANY 2
#define IMAGE_COMDAT_SELECT_SAME_SIZE 3
#define IMAGE_COMDAT_SELECT_EXACT_MATCH 4
#define IMAGE_COMDAT_SELECT_ASSOCIATIVE 5
#define IMAGE_COMDAT_SELECT_LARGEST 6

struct string_table_header_internal {

    unsigned long StringTableSize;

};

#define SIZEOF_struct_string_table_header_file 4
struct string_table_header_file {

    unsigned char StringTableSize[4]; /* Including the size of the header itself. */

};

struct IMAGE_EXPORT_DIRECTORY_internal {

    unsigned long ExportFlags; /* Reserved, must be 0. */
    unsigned long TimeDateStamp;
    unsigned short MajorVersion;
    unsigned short MinorVersion;
    unsigned long NameRVA;
    unsigned long OrdinalBase; /* Usually set to 1. */
    unsigned long AddressTableEntries;
    unsigned long NumberOfNamePointers;
    unsigned long ExportAddressTableRVA;
    unsigned long NamePointerRVA;
    unsigned long OrdinalTableRVA;

};

#define SIZEOF_struct_IMAGE_EXPORT_DIRECTORY_file 40
struct IMAGE_EXPORT_DIRECTORY_file {

    unsigned char ExportFlags[4];
    unsigned char TimeDateStamp[4];
    unsigned char MajorVersion[2];
    unsigned char MinorVersion[2];
    unsigned char NameRVA[4];
    unsigned char OrdinalBase[4];
    unsigned char AddressTableEntries[4];
    unsigned char NumberOfNamePointers[4];
    unsigned char ExportAddressTableRVA[4];
    unsigned char NamePointerRVA[4];
    unsigned char OrdinalTableRVA[4];

};

struct EXPORT_Address_Table_internal {
    unsigned long FunctionRVA;
};

#define SIZEOF_struct_EXPORT_Address_Table_file 4
struct EXPORT_Address_Table_file {
    unsigned char FunctionRVA[4];
};

struct EXPORT_Name_Pointer_Table_internal {
    unsigned long FunctionNameRVA;
};

#define SIZEOF_struct_EXPORT_Name_Pointer_Table_file 4
struct EXPORT_Name_Pointer_Table_file {
    unsigned char FunctionNameRVA[4];
};

struct EXPORT_Ordinal_Table_internal {
    unsigned short FunctionOrdinal;
};

#define SIZEOF_struct_EXPORT_Ordinal_Table_file 2
struct EXPORT_Ordinal_Table_file {
    unsigned char FunctionOrdinal[2];
};

struct IMPORT_Directory_Table_internal {

    unsigned long ImportNameTableRVA;
    unsigned long TimeDateStamp;
    unsigned long ForwarderChain;
    unsigned long NameRVA;
    unsigned long ImportAddressTableRVA;

};

#define SIZEOF_struct_IMPORT_Directory_Table_file 20
struct IMPORT_Directory_Table_file {

    unsigned char ImportNameTableRVA[4];
    unsigned char TimeDateStamp[4];
    unsigned char ForwarderChain[4];
    unsigned char NameRVA[4];
    unsigned char ImportAddressTableRVA[4];

};

struct IMAGE_BASE_RELOCATION_internal {

    unsigned long RVAOfBlock;
    unsigned long SizeOfBlock;

};

#define SIZEOF_struct_IMAGE_BASE_RELOCATION_file 8
struct IMAGE_BASE_RELOCATION_file {

    unsigned char RVAOfBlock[4];
    unsigned char SizeOfBlock[4];

};

#define BASE_RELOCATION_PAGE_SIZE 4096

/* Base relocation WORD:
 *  high 4 bits = Relocation type
 *  remaining 12 bits = Page offset
 */

#define IMAGE_REL_BASED_ABSOLUTE 0
#define IMAGE_REL_BASED_HIGH 1
#define IMAGE_REL_BASED_LOW 2
#define IMAGE_REL_BASED_HIGHLOW 3
#define IMAGE_REL_BASED_HIGHADJ 4
#define IMAGE_REL_BASED_MIPS_JMPADDR 5
#define IMAGE_REL_BASED_ARM_MOV32 5
#define IMAGE_REL_BASED_RISCV_HIGH20 5
#define IMAGE_REL_BASED_THUMB_MOV32 7
#define IMAGE_REL_BASED_RISCV_LOW12I 7
#define IMAGE_REL_BASED_RISCV_LOW12S 8
#define IMAGE_REL_BASED_MIPS_JMPADDR16 9
#define IMAGE_REL_BASED_DIR64 10

#define IMAGE_ARCHIVE_START "!<arch>\n"

struct IMAGE_ARCHIVE_MEMBER_HEADER_internal {

    char Name[16];
    char Date[12];
    char UserID[6];
    char GroupID[6];
    char Mode[8];
    char Size[10];
    char EndOfHeader[2];
#define IMAGE_ARCHIVE_MEMBER_HEADER_END_OF_HEADER "`\n"

};

#define SIZEOF_struct_IMAGE_ARCHIVE_MEMBER_HEADER_file 60
struct IMAGE_ARCHIVE_MEMBER_HEADER_file {

    char Name[16];
    char Date[12];
    char UserID[6];
    char GroupID[6];
    char Mode[8];
    char Size[10];
    char EndOfHeader[2];

};

#define IMAGE_ARCHIVE_LINKER_MEMBER_Name "/"
#define IMAGE_ARCHIVE_LONGNAMES_MEMBER_Name "//"

struct IMPORT_OBJECT_HEADER_internal {

    unsigned short Magic1; /* Must be IMAGE_FILE_MACHINE_UNKNOWN. */
    unsigned short Magic2;
#define IMPORT_OBJECT_HDR_MAGIC2 0xFFFF
    
    unsigned short Version;
    unsigned short Machine;
    unsigned long TimeDateStamp;
    unsigned long SizeOfData;
    unsigned short OrdinalHint;
    unsigned short Type; /* 2 bits import type, 3 bits import name type. */
    /* Followed by 2 null-terminated strings: symbol name and DLL name. */

};

#define SIZEOF_struct_IMPORT_OBJECT_HEADER_file 20
struct IMPORT_OBJECT_HEADER_file {

    unsigned char Magic1[2];
    unsigned char Magic2[2];
    unsigned char Version[2];
    unsigned char Machine[2];
    unsigned char TimeDateStamp[4];
    unsigned char SizeOfData[4];
    unsigned char OrdinalHint[2];
    unsigned char Type[2];

};

#define IMPORT_CODE 0
#define IMPORT_DATA 1
#define IMPORT_CONST 2

#define IMPORT_ORDINAL 0
#define IMPORT_NAME 1
#define IMPORT_NAME_NOPREFIX 2
#define IMPORT_NAME_UNDECORATE 3

