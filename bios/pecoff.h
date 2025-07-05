/*********************************************************************/
/*                                                                   */
/*  This Program Written by Alica Okano.                             */
/*  Released to the Public Domain as discussed here:                 */
/*  http://creativecommons.org/publicdomain/zero/1.0/                */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  pecoff.h - header file for PE/COFF support                       */
/*                                                                   */
/*********************************************************************/

/*
Documentation of format can be found here:
https://learn.microsoft.com/en-us/windows/win32/debug/pe-format
*/

/*
ARM example:
000080  50450000 C0010300 6628FF65 00000000  PE......f(.e....
000090  00000000 E0000E02 0B010238 00020000  ...........8....
0000A0  00040000 00000000 00100000 00100000  ................
0000B0  00000000 00000100 00100000 00020000  ................
0000C0  04000000 01000000 02000000 00000000  ................
optional header appears to start at 0x98
*/

#ifdef __LONG64__
typedef unsigned int U32;
#else
typedef unsigned long U32;
#endif

/* In image files there is signature "PE\0\0" at e_lfanew
 * after which is the COFF header. */
typedef struct {
    unsigned short Machine;
    unsigned short NumberOfSections;
    U32 TimeDateStamp;
    U32 PointerToSymbolTable; /* Deprecated, ignore. */
    U32 NumberOfSymbols; /* Deprecated, ignore. */
    unsigned short SizeOfOptionalHeader;
    unsigned short Characteristics;
} Coff_hdr;

/* Machine values. */
#define IMAGE_FILE_MACHINE_UNKNOWN 0
#define IMAGE_FILE_MACHINE_AMD64 0x8664
#define IMAGE_FILE_MACHINE_I386 0x14c
#define IMAGE_FILE_MACHINE_ARM 0x1c0
#define IMAGE_FILE_MACHINE_THUMB 0x1c2
#define IMAGE_FILE_MACHINE_M68K 0x268
#define IMAGE_FILE_MACHINE_ARMNT 0x1c4
#define IMAGE_FILE_MACHINE_ARM64 0xaa64

/* Characteristics flags. */
#define IMAGE_FILE_RELOCS_STRIPPED         0x0001
#define IMAGE_FILE_EXECUTABLE_IMAGE        0x0002
#define IMAGE_FILE_LARGE_ADDRESS_AWARE     0x0020
/* Reserved flag 0x0040. */
#define IMAGE_FILE_32BIT_MACHINE           0x0100
#define IMAGE_FILE_DEBUG_STRIPPED          0x0200
#define IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP 0x0400
#define IMAGE_FILE_NET_RUN_FROM_SWAP       0x0800
#define IMAGE_FILE_SYSTEM                  0x1000
#define IMAGE_FILE_DLL                     0x2000
#define IMAGE_FILE_UP_SYSTEM_ONLY          0x4000

/* PE/COFF optional header (magic is MAGIC_PE32). */
typedef struct {
    unsigned short Magic;
    unsigned char MajorLinkerVersion;
    unsigned char MinorLinkerVersion;
    U32 SizeOfCode;
    U32 SizeOfInitializedData;
    U32 SizeOfUninitializedData;
    U32 AddressOfEntryPoint; /* Relative to ImageBase. */
    U32 BaseOfCode;
#if !defined(TARGET_64BIT) || defined(SHIMCM32)
    U32 BaseOfData;
#endif
    /* Extension fields. */
#if defined(W32HACK) || defined(SHIMCM32)
    U32 ImageBase;
#else
    unsigned char *ImageBase;
#endif
    U32 SectionAlignment;
    U32 FileAlignment;
    unsigned short MajorOperatingSystemVersion;
    unsigned short MinorOperatingSystemVersion;
    unsigned short MajorImageVersion;
    unsigned short MinorImageVersion;
    unsigned short MajorSubsystemVersion;
    unsigned short MinorSubsystemVersion;
    U32 Win32VersionValue; /* Reserved, should be 0. */
    U32 SizeOfImage;
    U32 SizeOfHeaders;
    U32 CheckSum;
    unsigned short Subsystem;
    unsigned short DllCharacteristics;
    U32 SizeOfStackReserve;
#if defined(TARGET_64BIT) && !defined(SHIMCM32)
    U32 dummy1;
#endif
    U32 SizeOfStackCommit;
#if defined(TARGET_64BIT) && !defined(SHIMCM32)
    U32 dummy2;
#endif
    U32 SizeOfHeapReserve;
#if defined(TARGET_64BIT) && !defined(SHIMCM32)
    U32 dummy3;
#endif
    U32 SizeOfHeapCommit;
#if defined(TARGET_64BIT) && !defined(SHIMCM32)
    U32 dummy4;
#endif
    U32 LoaderFlags; /* Reserved, should be 0. */
    U32 NumberOfRvaAndSizes; /* Number of data directories. */
} Pe32_optional_hdr;

#define MAGIC_PE32     0x10B
#define MAGIC_PE32PLUS 0x20B

typedef struct {
    U32 VirtualAddress;
    U32 Size;
} IMAGE_DATA_DIRECTORY;

/* Indexes of data directories. */
#define DATA_DIRECTORY_EXPORT_TABLE 0
#define DATA_DIRECTORY_IMPORT_TABLE 1
#define DATA_DIRECTORY_REL 5

typedef struct {
    U32 Characteristics; /* Reserved. */
    U32 TimeDateStamp;
    unsigned short MajorVersion;
    unsigned short MinorVersion;
    U32 Name;
    U32 Base; /* Subtract from ordinals from Import tables. */
    U32 NumberOfFunctions;
    U32 NumberOfNames; /* How many functions are exported by name. */
    U32 AddressOfFunctions;
    U32 AddressOfNames;
    U32 AddressOfNameOrdinals;
} IMAGE_EXPORT_DIRECTORY;

typedef struct {
    U32 OriginalFirstThunk;
    U32 TimeDateStamp;
    U32 ForwarderChain;
    U32 Name; /* DLL name RVA. */
    U32 FirstThunk;
} IMAGE_IMPORT_DESCRIPTOR;

typedef struct {
    U32 PageRva;
    U32 BlockSize; /* Number of all bytes in the block. */
} Base_relocation_block;

/* Relocation types. */
#define IMAGE_REL_BASED_ABSOLUTE 0 /* Skip this relocation. */
#define IMAGE_REL_BASED_HIGHLOW  3
#define IMAGE_REL_BASED_ARM_MOV32 5
#define IMAGE_REL_BASED_THUMB_MOV32 7
#define IMAGE_REL_BASED_DIR64 10

typedef struct {
    unsigned char Name[8];
    U32 VirtualSize;
    U32 VirtualAddress;
    U32 SizeOfRawData;
    U32 PointerToRawData;
    U32 PointerToRelocations;
    U32 PointerToLinenumbers; /* Deprecated, ignore. */
    unsigned short NumberOfRelocations;
    unsigned short NumberOfLinenumbers; /* Deprecated, ignore. */
    U32 Characteristics;
} Coff_section;

/* Section Characteristics. */
#define IMAGE_SCN_CNT_CODE           0x00000020
#define IMAGE_SCN_INITIALIZED_DATA   0x00000040
#define IMAGE_SCN_UNINITIALIZED_DATA 0x00000080

