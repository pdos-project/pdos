/*********************************************************************/
/*                                                                   */
/*  This Program Written By Paul Edwards.                            */
/*  Released to the public domain.                                   */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  pcemul - emulate a PC                                            */
/*                                                                   */
/*********************************************************************/

#define PBEMUL 1

#include <stdio.h>
#include <stdlib.h>

#include <support.h>

#include <exeload.h>

#if PCEMUL
static size_t memsize = 640*1024L;
#else
static size_t memsize = 64*1024*1024L;
#endif

typedef unsigned int U32;

static unsigned char *base;
static unsigned char *p;
static int instr;

static int r1;
static int r2;

#define REG_AX 0
#define REG_BX 1
#define REG_CX 2
#define REG_DX 3
#define REG_DI 4
#define REG_SI 5
#define REG_SP 6
#define REG_IP 7
#define REG_SS 8
#define REG_DS 9
#define REG_ES 10
#define REG_CS 11
#define REG_BP 12
#define REG_AL 13
#define REG_AH 14
#define REG_BL 15
#define REG_BH 16
#define REG_CL 17
#define REG_CH 18
#define REG_DL 19
#define REG_DH 20
#define REG_EAX 21
#define REG_EBX 22
#define REG_ECX 23
#define REG_EDX 24
#define REG_EDI 25
#define REG_ESI 26
#define REG_ESP 27
#define REG_EIP 28
#define REG_EBP 29


typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned int DWORD;

static char *names[] = {
    "ax",
    "bx",
    "cx",
    "dx",
    "di",
    "si",
    "sp",
    "ip",
    "ss",
    "ds",
    "es",
    "cs",
    "bp",
    "al",
    "ah",
    "bl",
    "bh",
    "cl",
    "ch",
    "dl",
    "dh",
    "eax",
    "ebx",
    "ecx",
    "edx",
    "edi",
    "esi",
    "esp",
    "eip",
    "ebp",
};

int regsize = 0;

#define BYTE_SIZE 1
#define WORD_SIZE 2
#define DWORD_SIZE 4

#if 0
static BYTE *destb;
static BYTE *srcb;

static WORD *destw;
static WORD *srcw;
#endif

static void *dest;
static void *src;

#define TO_BYTE(d, s) (*(BYTE *)d = (s))
#define FROM_BYTE(s) (*(BYTE *)s)

#define TO_WORD(d, s) (*(WORD *)d = (s))
#define FROM_WORD(s) (*(WORD *)s)

#define TO_DWORD(d, s) (*(DWORD *)d = (s))
#define FROM_DWORD(s) (*(DWORD *)s)

#define GET_DEST_REG(y) \
   { \
   if (y == REG_SI) dest = &regs.x.si; \
   else if (y == REG_DI) dest = &regs.x.di; \
   else if (y == REG_AX) dest = &regs.x.ax; \
   else if (y == REG_BX) dest = &regs.x.bx; \
   else if (y == REG_CX) dest = &regs.x.cx; \
   else if (y == REG_DX) dest = &regs.x.dx; \
   else if (y == REG_BP) dest = &regs.x.bp; \
   else if (y == REG_AL) dest = &regs.h.al; \
   else if (y == REG_BL) dest = &regs.h.bl; \
   else if (y == REG_CL) dest = &regs.h.cl; \
   else if (y == REG_DL) dest = &regs.h.dl; \
   else if (y == REG_AH) dest = &regs.h.ah; \
   else if (y == REG_BH) dest = &regs.h.bh; \
   else if (y == REG_CH) dest = &regs.h.ch; \
   else if (y == REG_DH) dest = &regs.h.dh; \
   else if (y == REG_DS) dest = &sregs.ds; \
   else if (y == REG_ES) dest = &sregs.es; \
   else if (y == REG_SS) dest = &sregs.ss; \
   else if (y == REG_CS) dest = &sregs.cs; \
   else if (y == REG_SP) dest = &sp; \
   else if (y == REG_EAX) dest = &regs.d.eax; \
   else if (y == REG_EBX) dest = &regs.d.ebx; \
   else if (y == REG_ECX) dest = &regs.d.ecx; \
   else if (y == REG_EDX) dest = &regs.d.edx; \
   else if (y == REG_ESI) dest = &regs.d.esi; \
   else if (y == REG_EDI) dest = &regs.d.edi; \
   else if (y == REG_EBP) dest = &regs.d.ebp; \
   else if (y == REG_ESP) dest = &esp; \
   else { fprintf(logf, "unknown dest reg %d\n", y); exit(0); } \
   }

#define GET_SRC_REG(y) \
   { \
   if (y == REG_SI) src = &regs.x.si; \
   else if (y == REG_DI) src = &regs.x.di; \
   else if (y == REG_AX) src = &regs.x.ax; \
   else if (y == REG_BX) src = &regs.x.bx; \
   else if (y == REG_CX) src = &regs.x.cx; \
   else if (y == REG_DX) src = &regs.x.dx; \
   else if (y == REG_BP) src = &regs.x.bp; \
   else if (y == REG_AL) src = &regs.h.al; \
   else if (y == REG_BL) src = &regs.h.bl; \
   else if (y == REG_CL) src = &regs.h.cl; \
   else if (y == REG_DL) src = &regs.h.dl; \
   else if (y == REG_AH) src = &regs.h.ah; \
   else if (y == REG_BH) src = &regs.h.bh; \
   else if (y == REG_CH) src = &regs.h.ch; \
   else if (y == REG_DH) src = &regs.h.dh; \
   else if (y == REG_DS) src = &sregs.ds; \
   else if (y == REG_ES) src = &sregs.es; \
   else if (y == REG_SS) src = &sregs.ss; \
   else if (y == REG_CS) src = &sregs.cs; \
   else if (y == REG_SP) src = &sp; \
   else if (y == REG_EAX) src = &regs.d.eax; \
   else if (y == REG_EBX) src = &regs.d.ebx; \
   else if (y == REG_ECX) src = &regs.d.ecx; \
   else if (y == REG_EDX) src = &regs.d.edx; \
   else if (y == REG_ESI) src = &regs.d.esi; \
   else if (y == REG_EDI) src = &regs.d.edi; \
   else if (y == REG_EBP) src = &regs.d.ebp; \
   else if (y == REG_ESP) src = &esp; \
   else { fprintf(logf, "unknown src reg %d\n", y); exit(0); } \
   }

typedef size_t FLATAMT;

#define GET_FLATAMT(seg, offs) \
    (((FLATAMT)seg << 4) + offs)

#define BYTE_TO_MEM(toamt, val) \
    (base[toamt] = (BYTE)(val))

#define WORD_TO_MEM(toamt, val) \
    { \
        base[toamt] = (BYTE)(val & 0xff); \
        base[toamt + 1] = (BYTE)((val >> 8) & 0xff); \
    }

#define DWORD_TO_MEM(toamt, val) \
    { \
        base[toamt] = (BYTE)(val & 0xff); \
        base[toamt + 1] = (BYTE)((val >> 8) & 0xff); \
        base[toamt + 2] = (BYTE)((val >> 16) & 0xff); \
        base[toamt + 3] = (BYTE)((val >> 24) & 0xff); \
    }

#define BYTE_AT_MEM(fromamt) \
    ((BYTE)base[fromamt])

#define WORD_AT_MEM(fromamt) \
    ((WORD)((base[fromamt + 1] << 8) | base[fromamt]))

#define DWORD_AT_MEM(fromamt) \
    ((DWORD)( \
     (((DWORD)base[fromamt + 3]) << 24) \
    | (((DWORD)base[fromamt + 2]) << 16) \
    | (((DWORD)base[fromamt + 1]) << 8) \
    | base[fromamt]))

static int x1;
static int x2;
static int t;
static int i;
static int b;
static int d;

static int lt;
static int gt;
static int eq;
static int zero;
static int cflag = 0;
static int dflag = 0;

static int int_enable = 0;

static FILE *fp;

static FILE *logf;

static union REGS regs;
static struct SREGS sregs;
static WORD sp;

#if PBEMUL
static U32 esp;
#endif

static void doemul(void);
static void splitregs(unsigned int raw);
static void dobios(int intnum);

int main(int argc, char **argv)
{
    char *exe;
    unsigned char *entry_point;

    if (argc <= 1)
    {
        printf("usage: pb386em <exe>\n");
        printf("runs an OS that depends on a pseudo-bios\n");
        printf("will take a VHD later\n");
        return (EXIT_FAILURE);
    }

#if PCEMUL
    /* one day this will be r+b */
    fp = fopen(argv[1], "rb");
    if (fp == NULL)
    {
        printf("cannot open %s for reading\n", argv[1]);
        return (EXIT_FAILURE);
    }
#endif
    exe = argv[1];

    p = calloc(1, memsize);
    if (p == NULL)
    {
        printf("insufficient memory\n");
        return (EXIT_FAILURE);
    }
    base = p;

    /* first 1 MiB reserved for stack */
    p = base + 1024 * 1024L;
    /* tentatively a return to address 0 means a return here */
    /* and first and only parameter is the POS parms, which comes first */
    *((DWORD *)p - 1) = 1024 * 1024L;
    
    /* second 1 MiB reserved for BIOS use */
    /* first bit is the POS parms */
    /* the callbacks can't be real addresses - we need to
       detect them */
    {
        int x;
        
        for (x = 1; x < 200; x++)
        {
            *(U32 *)p = x;
            p += sizeof(U32);
        }
    }
    p = base + 2 * 1024 * 1024L;
    printf("about to load\n");
    /* the corrections that will be applied to the load are based on
       the actual address in memory. This will need to be changed to
       offsets from an arbitrary base address */
    if (exeloadDoload(&entry_point, exe, &p) != 0)
    {
        printf("failed to load %s\n", exe);
        exit(EXIT_FAILURE);
    }
    /* when the OS requests a block of memory, just give
       it 7 MiB above the base, allowing for a 5 MiB executable */
    /* That's probably way excessive */
    p = entry_point;
    logf = stdout;
    doemul();
    /* we are returning from a call, and it is our responsibility
       to clean up parameters, so we do that */
    esp += sizeof(DWORD);
    if (esp != 1024 * 1024L)
    {
       printf("unexpected early return\n");
    }
    printf("return code from called program is hex %08X\n", regs.d.eax);
#if PCEMUL
    fclose(fp);
#endif
    return (EXIT_SUCCESS);
}

static void spec_call(int val)
{
    printf("got a special call %d\n", val);
    if (val == 14) /* fwrite */
    {
        char *buf;
        size_t sz;
        size_t num;
        FILE *fq;
        
        buf = (char *)DWORD_AT_MEM(esp + sizeof(DWORD));
        printf("buf is %p\n", buf);
        /* If we start loading properly, we should be adding base here */

        sz = (size_t)DWORD_AT_MEM(esp + sizeof(DWORD)*2);
        printf("sz is %x\n", (int)sz);
        num = (size_t)DWORD_AT_MEM(esp + sizeof(DWORD)*3);
        printf("num is %x\n", (int)num);
        fq = (FILE *)DWORD_AT_MEM(esp + sizeof(DWORD)*4);
        printf("fq is %p\n", (int)fq);
        /* This should instead be a table of handles, where stdout
           should have been set to 1 in the POS structure */
        if (fq == (void *)9)
        {
            fq = stdout;
        }
        fwrite(buf, sz, num, fq);
    }
}

static void doemul(void)
{
#if PCEMUL
    unsigned char *watching = base + 0x18b50; /* 9e09 + 0x700; */ /* 0xa4c5; */
#endif

#if PBEMUL
    unsigned char *watching = base + 1024 * 1024L - 8 - 8 - 8;
#endif
        
#if PCEMUL
    regs.h.dl = 0x80;
    p = base + 0x7c00;
#endif
    sregs.cs = 0;
    sregs.ss = 0;
#if PCEMUL
    sp = 0x7c00;
#else
    esp = 1024 * 1024L - 8; /* the -8 is for the parm, and return address */
#endif
    
#if PCEMUL
    fread(base + 0x7c00, 1, 512, fp);
#endif

    while (1)
    {
        instr = *p;
        fprintf(logf, "\n");
        fprintf(logf, "instr is %02X at %08lX watching %05lX %02X %02X %02X %02X\n",
               instr, (long)(p - base),
               (long)(watching - base),
               watching[0], watching[1], watching[2], watching[3]);
#if PCEMUL
        fprintf(logf,
                "ax %04X, bx %04X, cx %04X, dx %04X, di %04X, si %04X, bp %04X\n",
                regs.x.ax, regs.x.bx, regs.x.cx, regs.x.dx,
                regs.x.di, regs.x.si, regs.x.bp);
        fprintf(logf,
                "ss %04X, sp %04X, cs %04X, ip %04X, ds %04X, es %04X\n",
                sregs.ss, sp, sregs.cs, (unsigned int)(p - base), sregs.ds, sregs.es);
#endif

#if PBEMUL
        fprintf(logf,
                "eax %08X, ebx %08X, ecx %08X, edx %08X, edi %08X, esi %08X\n",
                regs.d.eax, regs.d.ebx, regs.d.ecx, regs.d.edx,
                regs.d.edi, regs.d.esi);
        fprintf(logf,
                "ebp %08X, esp %08X\n",
                regs.d.ebp, esp);
#endif

        if (instr == 0xfa)
        {
            fprintf(logf, "cli\n");
            int_enable = 0;
            p++;
        }
        /* FC              0041      cld */
        else if (instr == 0xfc)
        {
            fprintf(logf, "cld\n");
            dflag = 0;
            p++;
        }
        /* 33C0            0001     xor ax,ax */
        else if ((instr == 0x33) && (p[1] == 0xc0))
        {
            fprintf(logf, "xor ax,ax\n");
            regs.x.ax ^= regs.x.ax;
            p += 2;
        }
        /* 33D2            013A      xor  dx, dx */
        /* 33C9            00C0      xor cx, cx */
        else if (instr == 0x33)
        {
            int r;
            
            if (p[1] == 0xd2)
            {
                r = REG_DX;
            }
            else if (p[1] == 0xc9)
            {
                r = REG_CX;
            }
            else
            {
                printf("unknown register %x\n", p[1]);
                exit(EXIT_FAILURE);
            }
            fprintf(logf, "xor %s,%s\n", names[r], names[r]);
            
            GET_SRC_REG(r);
            dest = src;
            TO_WORD(dest, FROM_WORD(src) ^ FROM_WORD(src));
            p += 2;
        }
        /* 8E04            0013     mov es, [si + 0] */
        else if ((instr == 0x8e) && (p[1] == 0x04))
        {
            WORD seg;
            WORD offs;
            FLATAMT fromamt;
            int r;
            int r2;
                
            regsize = WORD_SIZE;
            if (p[1] == 0x04)
            {
                r = REG_SI;
                r2 = REG_ES;
            }
            else
            {
                printf("unknown register %x\n", p[1]);
                exit(EXIT_FAILURE);
            }
            GET_SRC_REG(r);
            offs = FROM_WORD(src);
            
            GET_SRC_REG(REG_DS);
            seg = FROM_WORD(src);

            fromamt = GET_FLATAMT(seg, offs);
            
            fprintf(logf, "mov %s, [%s+0%02X]\n", names[r2], names[r], 0);
            
            GET_DEST_REG(r2);
            TO_WORD(dest, WORD_AT_MEM(fromamt));
            
            p += 2;
        }
        
        /* 8E4406          000F     mov es, [si + 6] */
        else if ((instr == 0x8e) && (p[1] == 0x44))
        {
            WORD seg;
            WORD offs;
            FLATAMT fromamt;
            int r;
            int r2;
                
            regsize = WORD_SIZE;
            if (p[1] == 0x44)
            {
                r = REG_SI;
                r2 = REG_ES;
            }
            else
            {
                printf("unknown register %x\n", p[1]);
                exit(EXIT_FAILURE);
            }
            GET_SRC_REG(r);
            offs = FROM_WORD(src);
            
            offs += p[2];

            GET_SRC_REG(REG_DS);
            seg = FROM_WORD(src);

            fromamt = GET_FLATAMT(seg, offs);
            
            fprintf(logf, "mov %s, [%s+0%02X]\n", names[r2], names[r], p[2]);
            
            GET_DEST_REG(r2);
            TO_WORD(dest, WORD_AT_MEM(fromamt));
            
            p += 3;
        }
        
        /* 8ED8            0003     mov ds,ax */
        /* 8EC0            0005     mov es,ax */
        /* 8EDA            0023     mov ds, dx */
        else if (instr == 0x8e)
        {
            regsize = WORD_SIZE;
            splitregs(*(p + 1));
            fprintf(logf, "mov %s,%s\n", names[r1], names[r2]);

            TO_WORD(dest, FROM_WORD(src));
            p += 2;
        }
        /* 8AD6            0066      mov dl,dh */
        else if ((instr == 0x8a) && (p[1] == 0xd6))
        {
            regsize = BYTE_SIZE;
            /* splitregs(*(p + 1)); */
            fprintf(logf, "mov %s,%s\n", names[REG_DL], names[REG_DH]);

            GET_DEST_REG(REG_DL);
            GET_SRC_REG(REG_DH);
            TO_BYTE(dest, FROM_BYTE(src));
            p += 2;
        }
        /* 8ACA            0136      mov  cl,  dl */
        else if ((instr == 0x8a) && (p[1] == 0xca))
        {
            regsize = BYTE_SIZE;
            fprintf(logf, "mov %s,%s\n", names[REG_CL], names[REG_DL]);

            GET_DEST_REG(REG_CL);
            GET_SRC_REG(REG_DL);
            TO_BYTE(dest, FROM_BYTE(src));
            p += 2;
        }
        /* 8AF2            0140      mov  dh,  dl */
        else if ((instr == 0x8a) && (p[1] == 0xf2))
        {
            regsize = BYTE_SIZE;
            fprintf(logf, "mov %s,%s\n", names[REG_DH], names[REG_DL]);

            GET_DEST_REG(REG_DH);
            GET_SRC_REG(REG_DL);
            TO_BYTE(dest, FROM_BYTE(src));
            p += 2;
        }
        /* 8AF0            0143      mov  dh, al */
        else if ((instr == 0x8a) && (p[1] == 0xf0))
        {
            regsize = BYTE_SIZE;
            fprintf(logf, "mov %s,%s\n", names[REG_DH], names[REG_AL]);

            GET_DEST_REG(REG_DH);
            GET_SRC_REG(REG_AL);
            TO_BYTE(dest, FROM_BYTE(src));
            p += 2;
        }
        /* 8AE6            014D      mov  ah, dh */
        else if ((instr == 0x8a) && (p[1] == 0xe6))
        {
            regsize = BYTE_SIZE;
            /* splitregs(*(p + 1)); */
            fprintf(logf, "mov %s,%s\n", names[REG_AH], names[REG_DH]);

            GET_DEST_REG(REG_AH);
            GET_SRC_REG(REG_DH);
            TO_BYTE(dest, FROM_BYTE(src));
            p += 2;
        }
        /* 8A07            00AD           mov     al,[bx] */
        else if ((instr == 0x8a) && (p[1] == 0x07))
        {
            FLATAMT fromamt;
            WORD seg;
            WORD offs;
            
            regsize = BYTE_SIZE;
            fprintf(logf, "mov %s,[%s]\n", names[REG_AL], names[REG_BX]);

            GET_SRC_REG(REG_BX);
            offs = FROM_WORD(src);

            GET_SRC_REG(REG_DS);
            seg = FROM_WORD(src);

            fromamt = GET_FLATAMT(seg, offs);

            GET_DEST_REG(REG_AL);
            TO_BYTE(dest, BYTE_AT_MEM(fromamt));
            p += 2;
        }
        /* 8A4600          00B0     mov al,[bp] */
        /* 8A4608          009E             mov al,byte ptr character */
        else if ((instr == 0x8a) && (p[1] == 0x46))
        {
            FLATAMT fromamt;
            WORD seg;
            WORD offs;
            
            regsize = BYTE_SIZE;
            if (p[2] >= 0x80)
            {
                printf("is this signed?\n");
                exit(EXIT_FAILURE);
            }
            fprintf(logf, "mov %s,[%s+0%02Xh]\n",
                    names[REG_AL], names[REG_BP], p[2]);

            GET_SRC_REG(REG_BP);
            offs = FROM_WORD(src);
            offs += p[2];

            /* I have confirmed via testing that SS should be used
               to access bp. The mbr.asm potential bug is where it
               loads bp and then seemingly relies on ds rather than
               ss to point to the error message to be printed. The
               potential bug is not normally seen because the data
               is normally correct. So need to do a test by zapping
               the AA55 signature to invoke an error and see if the
               message is printed, and if so, delete this comment
               as there is no bug. Also other code will need to
               change as I have used DS when I should have used SS
               in multiple places and it's not trivial to change
               and nor is it a priority. */
            /* I have confirmed that the MBR works fine and prints
               an error message correctly, so I must have misdiagnosed */
            GET_SRC_REG(REG_SS);

            seg = FROM_WORD(src);

            fromamt = GET_FLATAMT(seg, offs);

            GET_DEST_REG(REG_AL);
            TO_BYTE(dest, BYTE_AT_MEM(fromamt));
            p += 3;
        }
        /* 8A16B501 mov  dl, [BootDisk] */
        else if ((instr == 0x8a) && (p[1] == 0x16))
        {
            FLATAMT fromamt;
            WORD seg;
            WORD offs;
            
            regsize = BYTE_SIZE;

            offs = (p[3] << 8) | p[2];

            GET_SRC_REG(REG_DS);
            seg = FROM_WORD(src);

            fromamt = GET_FLATAMT(seg, offs);

            fprintf(logf, "mov %s,[0%05X]\n", names[REG_DL], fromamt);
            GET_DEST_REG(REG_DL);
            TO_BYTE(dest, BYTE_AT_MEM(fromamt));
            p += 4;
        }
        /* A11600          00BB      mov ax, [FatSize16] */
        else if (instr == 0xa1)
        {
            FLATAMT fromamt;
            WORD seg;
            WORD offs;
            
            regsize = WORD_SIZE;

            offs = (p[2] << 8) | p[1];

            GET_SRC_REG(REG_DS);
            seg = FROM_WORD(src);

            fromamt = GET_FLATAMT(seg, offs);

            fprintf(logf, "mov %s,[0%05Xh]\n", names[REG_AX], fromamt);
            GET_DEST_REG(REG_AX);
            TO_WORD(dest, WORD_AT_MEM(fromamt));
            p += 3;
        }
        /* A3130C          0016           mov     _dseg,ax */
        else if (instr == 0xa3)
        {
            FLATAMT toamt;
            WORD seg;
            WORD offs;
            
            regsize = WORD_SIZE;

            offs = (p[2] << 8) | p[1];

            GET_SRC_REG(REG_DS);
            seg = FROM_WORD(src);

            toamt = GET_FLATAMT(seg, offs);

            fprintf(logf, "mov 0%05Xh, %s\n", toamt, names[REG_AX]);
            GET_SRC_REG(REG_AX);
            WORD_TO_MEM(toamt, FROM_WORD(src));
            p += 3;
        }
        /* D3E8            014A      shr  ax, cl */
        /* D3E0            0412           shl     ax,cl */
        /* D3F8            0633           sar     ax,cl */
        else if (instr == 0xd3)
        {
            BYTE val;
            WORD val2;
            int left;
            int arith = 0;
            
            regsize = WORD_SIZE;
            if (p[1] == 0xe8)
            {
                left = 0;
            }
            else if (p[1] ==0xf8)
            {
                left = 0;
                arith = 1;
            }
            else if (p[1] == 0xe0)
            {
                left = 1;
            }
            else
            {
                printf("unknown register %x\n", p[1]);
                exit(EXIT_FAILURE);
            }
            if (left)
            {
                fprintf(logf, "shl %s,%s\n", names[REG_AX], names[REG_CL]);
            }
            else if (arith)
            {
                fprintf(logf, "sar %s,%s - converted to shr for now\n",
                        names[REG_AX], names[REG_CL]);
            }
            else
            {
                fprintf(logf, "shr %s,%s\n", names[REG_AX], names[REG_CL]);
            }

            GET_SRC_REG(REG_CL);
            val = FROM_BYTE(src);
            
            GET_SRC_REG(REG_AX);
            val2 = FROM_WORD(src);
            
            if (left)
            {
                val2 <<= val;
            }
            else
            {
                val2 >>= val;
            }
            
            dest = src;
            TO_WORD(dest, val2);
            p += 2;
        }
        /* 0BC8            0150           or      cx,ax */
        /* 0BC1            0415           or      ax,cx */
        /* 0BC0            0101           or      ax,ax */
        else if (instr == 0x0b)
        {
            WORD val;
            WORD val2;
            int r1;
            int r2;
            
            regsize = WORD_SIZE;
            if (p[1] == 0xc8)
            {
                r1 = REG_AX;
                r2 = REG_CX;
            }
            else if (p[1] == 0xc1)
            {
                r1 = REG_CX;
                r2 = REG_AX;
            }
            else if (p[1] == 0xc0)
            {
                r1 = REG_AX;
                r2 = REG_AX;
            }
            else
            {
                printf("unknown register %x\n", p[1]);
                exit(EXIT_FAILURE);
            }
            fprintf(logf, "or %s,%s\n", names[r2], names[r1]);

            GET_SRC_REG(r1);
            val = FROM_WORD(src);
            
            GET_SRC_REG(r2);
            val2 = FROM_WORD(src);
            
            val2 |= val;
            
            if (val2 == 0)
            {
                zero = 1;
            }
            else
            {
                zero = 0;
            }
            
            dest = src;
            TO_WORD(dest, val2);
            p += 2;
        }
        /* 39C1            006E           cmp     cx,ax */
        else if (instr == 0x39)
        {
            WORD val;
            WORD val2;
            
            regsize = WORD_SIZE;
            if (p[1] != 0xc1)
            {
                printf("unknown register %x\n", p[1]);
                exit(EXIT_FAILURE);
            }
            fprintf(logf, "cmp %s,%s\n", names[REG_CX], names[REG_AX]);

            GET_SRC_REG(REG_AX);
            val = FROM_WORD(src);
            
            GET_SRC_REG(REG_CX);
            val2 = FROM_WORD(src);

            /* I think these are the right way around */
            zero = 0;
            if (val2 == val)
            {
                lt = 0;
                gt = 0;
                eq = 1;
                zero = 1; /* just a guess */
            }
            else if (val2 < val)
            {
                lt = 1;
                gt = 0;
                eq = 0;
            }
            else
            {
                lt = 0;
                gt = 1;
                eq = 0;
            }
            p += 2;
        }
        /* 014EFC   add     [bp-4],cx */
        else if ((instr == 0x01) && (p[1] == 0x4e))
        {
            WORD val;
            WORD val2;
            WORD oldval2;
            WORD seg;
            WORD offs;
            FLATAMT fromamt;
            int r = REG_CX;
            int r2 = REG_BP;
                
            regsize = WORD_SIZE;

            GET_SRC_REG(REG_CX);
            val = FROM_WORD(src);
                        
            GET_SRC_REG(REG_DS);
            seg = FROM_WORD(src);

            GET_SRC_REG(REG_BP);
            offs = FROM_WORD(src);
            
            fromamt = GET_FLATAMT(seg, offs);
            
            if (p[2] >= 0x80)
            {
                fromamt -= (0x100U - p[2]);
                fprintf(logf, "add [%s-0%02Xh],%s\n",
                        names[r2], (0x100U - p[2]), names[r]);
            }
            else
            {
                fromamt += p[2];
                fprintf(logf, "add [%s+0%02Xh],%s\n",
                        names[r2], p[2], names[r]);
            }

            val2 = WORD_AT_MEM(fromamt);

            oldval2 = val2;
            val2 += val;
            if (val2 < oldval2)
            {
                cflag = 1;
            }
            else
            {
                cflag = 0;
            }
            WORD_TO_MEM(fromamt, val2);
            p += 3;
        }
        /* 03060E00 add ax, [ReservedSectors] */
        /* 031E0B00 add bx,  [BytesPerSector] */
        else if ((instr == 0x03) && ((p[1] == 0x06) || (p[1] == 0x1e)))
        {
            WORD val;
            WORD val2;
            WORD oldval2;
            WORD seg;
            WORD offs;
            FLATAMT fromamt;
            int r;
                
            regsize = WORD_SIZE;
            
            if (p[1] == 0x06)
            {
                r = REG_AX;
            }
            else if (p[1] == 0x1e)
            {
                r = REG_BX;
            }
            
            GET_SRC_REG(REG_DS);
            seg = FROM_WORD(src);

            offs = (p[3] << 8) | p[2];

            fromamt = GET_FLATAMT(seg, offs);
            val = WORD_AT_MEM(fromamt);

            fprintf(logf, "add %s, [%05Xh]\n", names[r], fromamt);
            
            GET_SRC_REG(r);

            val2 = FROM_WORD(src);
            oldval2 = val2;
            val2 += val;
            if (val2 < oldval2)
            {
                cflag = 1;
            }
            else
            {
                cflag = 0;
            }
            dest = src;
            TO_WORD(dest, val2);
            p += 4;
        }
        /* 03C3            00DC      add ax, bx */
        /* 03C1            00A7           add     ax,cx */
        else if (instr == 0x03)
        {
            WORD val;
            WORD val2;
            WORD oldval2;
            int r;
            
            regsize = WORD_SIZE;
            if (p[1] == 0xc3)
            {
                r = REG_BX;
            }
            else if (p[1] == 0xc1)
            {
                r = REG_CX;
            }
            else
            {
                printf("unknown register %x\n", p[1]);
                exit(EXIT_FAILURE);
            }
            fprintf(logf, "add %s,%s\n", names[REG_AX], names[r]);

            GET_SRC_REG(r);
            val = FROM_WORD(src);
            
            GET_SRC_REG(REG_AX);
            val2 = FROM_WORD(src);
            
            oldval2 = val2;
            val2 += val;
            if (val2 < oldval2)
            {
                cflag = 1;
            }
            else
            {
                cflag = 0;
            }
            
            dest = src;
            TO_WORD(dest, val2);
            p += 2;
        }
        /* 13161E00   adc dx, [HiddenSectors_High] */
        else if ((instr == 0x13) && (p[1] == 0x16))
        {
            WORD val;
            WORD val2;
            WORD oldval2;
            WORD seg;
            WORD offs;
            FLATAMT fromamt;
                
            regsize = WORD_SIZE;
            
            GET_SRC_REG(REG_DS);
            seg = FROM_WORD(src);

            offs = (p[3] << 8) | p[2];

            fromamt = GET_FLATAMT(seg, offs);
            val = WORD_AT_MEM(fromamt);

            fprintf(logf, "adc %s, [%05Xh]\n", names[REG_DX], fromamt);
            
            GET_SRC_REG(REG_DX);

            val2 = FROM_WORD(src);
            oldval2 = val2;
            val2 += (val + cflag);
            if (val2 < oldval2)
            {
                cflag = 1;
            }
            else
            {
                cflag = 0;
            }
            dest = src;
            TO_WORD(dest, val2);
            p += 4;
        }
        /* 13D1            00DE      adc dx, cx */
        else if (instr == 0x13)
        {
            WORD val;
            WORD val2;
            WORD oldval2;
            
            regsize = WORD_SIZE;
            if (p[1] != 0xd1)
            {
                printf("unknown register %x\n", p[1]);
                exit(EXIT_FAILURE);
            }
            fprintf(logf, "adc %s,%s\n", names[REG_DX], names[REG_CX]);

            GET_SRC_REG(REG_CX);
            val = FROM_WORD(src);
            
            GET_SRC_REG(REG_DX);
            val2 = FROM_WORD(src);
            
            oldval2 = val2;
            val2 += (val + cflag);
            if (val2 < oldval2)
            {
                cflag = 1;
            }
            else
            {
                cflag = 0;
            }
            
            dest = src;
            TO_WORD(dest, val2);
            p += 2;
        }

#if PBEMUL
        /* B8050000 mov eax, 5 */
        else if (instr == 0xb8)
        {
            DWORD val;

            regsize = DWORD_SIZE;
            val = DWORD_AT_MEM(p + 1 - base);
            
            GET_DEST_REG(REG_EAX);
            
            fprintf(logf, "mov eax 0%08Xh\n", val);
            
            TO_DWORD(dest, val);
            p += 5;
        }
#endif

#if PCEMUL
        /* B85000          0007     mov ax, 050h */
        else if (instr == 0xb8)
        {
            WORD val;

            regsize = WORD_SIZE;
            r1 = REG_AX;
            dest = &regs.x.ax;
            val = p[2] << 8 | p[1];
            fprintf(logf, "mov %s,0%04Xh\n", names[r1], (unsigned int)val);
            TO_WORD(dest, val);
            p += 3;
        }
#endif

        /* BB007C          008A     mov bx, 07c00h */
        else if (instr == 0xbb)
        {
            WORD val;

            regsize = WORD_SIZE;
            GET_DEST_REG(REG_BX);
            val = p[2] << 8 | p[1];
            fprintf(logf, "mov %s,0%04Xh\n", names[REG_BX], (unsigned int)val);
            TO_WORD(dest, val);
            p += 3;
        }
        /* BA0000          00A9      mov  dx, 0 */
        else if (instr == 0xba)
        {
            WORD val;

            regsize = WORD_SIZE;
            GET_DEST_REG(REG_DX);
            val = p[2] << 8 | p[1];
            fprintf(logf, "mov %s,0%04Xh\n", names[REG_DX], (unsigned int)val);
            TO_WORD(dest, val);
            p += 3;
        }
        /* BC0077          000C     mov sp,07700h */
        else if (instr == 0xbc)
        {
            WORD val;

            regsize = WORD_SIZE;
            r1 = REG_SP;
            dest = &sp;
            val = p[2] << 8 | p[1];
            fprintf(logf, "mov %s,0%04Xh\n", names[r1], (int)val);
            TO_WORD(dest, val);
            p += 3;
        }
        /* FB              000F     sti */
        else if (instr == 0xfb)
        {
            fprintf(logf, "sti\n");
            int_enable = 1;
            p++;
        }
        /* BE007C          0010     mov si,07c00h */
        else if (instr == 0xbe)
        {
            WORD val;

            regsize = WORD_SIZE;
            GET_DEST_REG(REG_SI);
            val = p[2] << 8 | p[1];
            fprintf(logf, "mov %s,0%04Xh\n", names[REG_SI], (int)val);
            TO_WORD(dest, val);
            p += 3;
        }
        /* BF0006          0013     mov di,0600h */
        else if (instr == 0xbf)
        {
            WORD val;

            regsize = WORD_SIZE;
            GET_DEST_REG(REG_DI);
            val = p[2] << 8 | p[1];
            fprintf(logf, "mov %s,0%04Xh\n", names[REG_DI], (int)val);
            TO_WORD(dest, val);
            p += 3;
        }
        /* B90001          0016     mov cx,0100h */
        else if (instr == 0xb9)
        {
            WORD val;

            regsize = WORD_SIZE;
            GET_DEST_REG(REG_CX);
            val = p[2] << 8 | p[1];
            fprintf(logf, "mov %s,0%04Xh\n", names[REG_CX], (int)val);
            TO_WORD(dest, val);
            p += 3;
        }
        /* F3A5            0019     rep movsw */
        /* di is target. guessing target segment register is es.
        /* si is source. guessing source segment register is ds */
        /* length is cx */
        else if (instr == 0xf3)
        {
            if (p[1] == 0xa5)
            {
                WORD len;
                WORD seg;
                WORD offs;
                FLATAMT fromamt;
                FLATAMT toamt;
                unsigned char *from;
                unsigned char *to;
                
                GET_SRC_REG(REG_CX);
                len = FROM_WORD(src);
                
                GET_SRC_REG(REG_ES);
                seg = FROM_WORD(src);
                
                GET_SRC_REG(REG_DI);
                offs = FROM_WORD(src);

                toamt = GET_FLATAMT(seg, offs);
                to = base + toamt;
                
                GET_SRC_REG(REG_DS);
                seg = FROM_WORD(src);
                
                GET_SRC_REG(REG_SI);
                offs = FROM_WORD(src);
                
                fromamt = GET_FLATAMT(seg, offs);
                from = base + fromamt;

#if 0
                printf("base %p, from %p, to %p, len %d\n", base, from, to, len * 2);
                exit(0);
#endif
                fprintf(logf, "rep movsw\n");
                memcpy(to, from, len * 2);
                
                p += 2;
            }
            else
            {
                fprintf(logf, "unknown f3\n");
                exit(EXIT_FAILURE);
            }
        }
        /* 50              001E     push ax */
        else if (instr == 0x50)
        {
            WORD val;
            WORD seg;
            WORD offs;
            FLATAMT toamt;
                
            regsize = WORD_SIZE;
            GET_SRC_REG(REG_AX);
            val = FROM_WORD(src);

            GET_SRC_REG(REG_SS);
            seg = FROM_WORD(src);
                
            GET_SRC_REG(REG_SP);
            offs = FROM_WORD(src);

            toamt = GET_FLATAMT(seg, offs);
            toamt -= 2;

            fprintf(logf, "push %s\n", names[REG_AX]);
            dest = src;
            TO_WORD(dest, FROM_WORD(src) - 2);
            WORD_TO_MEM(toamt, val);
            p++;
        }
        /* 51              0147      push cx */
        else if (instr == 0x51)
        {
            WORD val;
            WORD seg;
            WORD offs;
            FLATAMT toamt;
                
            regsize = WORD_SIZE;
            GET_SRC_REG(REG_CX);
            val = FROM_WORD(src);

            GET_SRC_REG(REG_SS);
            seg = FROM_WORD(src);
                
            GET_SRC_REG(REG_SP);
            offs = FROM_WORD(src);

            toamt = GET_FLATAMT(seg, offs);
            toamt -= 2;

            fprintf(logf, "push %s\n", names[REG_CX]);
            dest = src;
            TO_WORD(dest, FROM_WORD(src) - 2);
            WORD_TO_MEM(toamt, val);
            p++;
        }
        /* 52              0142      push dx */
        else if (instr == 0x52)
        {
            WORD val;
            WORD seg;
            WORD offs;
            FLATAMT toamt;
                
            regsize = WORD_SIZE;
            GET_SRC_REG(REG_DX);
            val = FROM_WORD(src);

            GET_SRC_REG(REG_SS);
            seg = FROM_WORD(src);
                
            GET_SRC_REG(REG_SP);
            offs = FROM_WORD(src);

            toamt = GET_FLATAMT(seg, offs);
            toamt -= 2;

            fprintf(logf, "push %s\n", names[REG_DX]);
            dest = src;
            TO_WORD(dest, FROM_WORD(src) - 2);
            WORD_TO_MEM(toamt, val);
            p++;
        }
        /* 53              0164      push bx */
        else if (instr == 0x53)
        {
            WORD val;
            WORD seg;
            WORD offs;
            FLATAMT toamt;
                
            regsize = WORD_SIZE;
            GET_SRC_REG(REG_BX);
            val = FROM_WORD(src);

            GET_SRC_REG(REG_SS);
            seg = FROM_WORD(src);
                
            GET_SRC_REG(REG_SP);
            offs = FROM_WORD(src);

            toamt = GET_FLATAMT(seg, offs);
            toamt -= 2;

            fprintf(logf, "push %s\n", names[REG_BX]);
            dest = src;
            TO_WORD(dest, FROM_WORD(src) - 2);
            WORD_TO_MEM(toamt, val);
            p++;
        }

#if PBEMUL
        /* 68 00 00 00 00 push $LC0 */
        else if (instr == 0x68)
        {
            DWORD val;
            FLATAMT toamt;

            regsize = DWORD_SIZE;
            val = DWORD_AT_MEM(p + 1 - base);
            
            GET_SRC_REG(REG_ESP);
            toamt = FROM_DWORD(src);
            toamt -= sizeof(DWORD);
            
            fprintf(logf, "push 0%08Xh\n", val);
            
            dest = src;
            TO_DWORD(dest, FROM_DWORD(src) - sizeof(DWORD));
            DWORD_TO_MEM(toamt, val);
            p += 5;
        }
#endif

#if PBEMUL
        /* 6A01 push $1 */
        else if (instr == 0x6A)
        {
            DWORD val;
            FLATAMT toamt;

            regsize = DWORD_SIZE;
            val = p[1];
            
            GET_SRC_REG(REG_ESP);
            toamt = FROM_DWORD(src);
            toamt -= sizeof(DWORD);
            
            fprintf(logf, "push 0%02Xh\n", val);
            
            dest = src;
            TO_DWORD(dest, FROM_DWORD(src) - sizeof(DWORD));
            DWORD_TO_MEM(toamt, val);
            p += 2;
        }
#endif

#if PBEMUL
        /* C9 leave */
        else if (instr == 0xc9)
        {
            DWORD val;
            FLATAMT fromamt;

            regsize = DWORD_SIZE;
            GET_SRC_REG(REG_EBP);
            val = FROM_DWORD(src);
            
            GET_DEST_REG(REG_ESP);
            TO_DWORD(dest, val);
            
            GET_SRC_REG(REG_ESP);
            fromamt = FROM_DWORD(src);
            val = DWORD_AT_MEM(fromamt);

            dest = src;
            TO_DWORD(dest, FROM_DWORD(src) + sizeof(DWORD));
            
            GET_DEST_REG(REG_EBP);
            TO_DWORD(dest, val);
            
            fprintf(logf, "leave\n");
            
            p++;
        }
#endif

        /* 55              0000     _main:        push    bp */
        else if (instr == 0x55)
        {
#if PCEMUL
            WORD val;
            WORD seg;
            WORD offs;
            FLATAMT toamt;
                
            regsize = WORD_SIZE;
            GET_SRC_REG(REG_BP);
            val = FROM_WORD(src);

            GET_SRC_REG(REG_SS);
            seg = FROM_WORD(src);
                
            GET_SRC_REG(REG_SP);
            offs = FROM_WORD(src);

            toamt = GET_FLATAMT(seg, offs);
            toamt -= 2;

            fprintf(logf, "push %s\n", names[REG_BP]);
            dest = src;
            TO_WORD(dest, FROM_WORD(src) - 2);
            WORD_TO_MEM(toamt, val);
            p++;
#endif
#if PBEMUL
            DWORD val;
            FLATAMT toamt;

            regsize = DWORD_SIZE;
            GET_SRC_REG(REG_EBP);
            val = FROM_DWORD(src);
            
            GET_SRC_REG(REG_ESP);
            toamt = FROM_DWORD(src);
            toamt -= sizeof(DWORD);
            
            fprintf(logf, "push %s\n", names[REG_EBP]);
            
            dest = src;
            TO_DWORD(dest, FROM_DWORD(src) - sizeof(DWORD));
            DWORD_TO_MEM(toamt, val);
            p++;
#endif
        }
        /* 56  push si */
        else if (instr == 0x56)
        {
            WORD val;
            WORD seg;
            WORD offs;
            FLATAMT toamt;
                
            regsize = WORD_SIZE;
            GET_SRC_REG(REG_SI);
            val = FROM_WORD(src);

            GET_SRC_REG(REG_SS);
            seg = FROM_WORD(src);
                
            GET_SRC_REG(REG_SP);
            offs = FROM_WORD(src);

            toamt = GET_FLATAMT(seg, offs);
            toamt -= 2;

            fprintf(logf, "push %s\n", names[REG_SI]);
            dest = src;
            TO_WORD(dest, FROM_WORD(src) - 2);
            WORD_TO_MEM(toamt, val);
            p++;
        }
        /* 57              0042     push di */
        else if (instr == 0x57)
        {
            WORD val;
            WORD seg;
            WORD offs;
            FLATAMT toamt;
                
            regsize = WORD_SIZE;
            GET_SRC_REG(REG_DI);
            val = FROM_WORD(src);

            GET_SRC_REG(REG_SS);
            seg = FROM_WORD(src);
                
            GET_SRC_REG(REG_SP);
            offs = FROM_WORD(src);

            toamt = GET_FLATAMT(seg, offs);
            toamt -= 2;

            fprintf(logf, "push %s\n", names[REG_DI]);
            dest = src;
            TO_WORD(dest, FROM_WORD(src) - 2);
            WORD_TO_MEM(toamt, val);
            p++;
        }
        /* 06              0167      push es */
        else if (instr == 0x06)
        {
            WORD val;
            WORD seg;
            WORD offs;
            FLATAMT toamt;
                
            regsize = WORD_SIZE;
            GET_SRC_REG(REG_ES);
            val = FROM_WORD(src);

            GET_SRC_REG(REG_SS);
            seg = FROM_WORD(src);
                
            GET_SRC_REG(REG_SP);
            offs = FROM_WORD(src);

            toamt = GET_FLATAMT(seg, offs);
            toamt -= 2;

            fprintf(logf, "push %s\n", names[REG_ES]);
            dest = src;
            TO_WORD(dest, FROM_WORD(src) - 2);
            WORD_TO_MEM(toamt, val);
            p++;
        }
        /* 1E              003E     push ds */
        else if (instr == 0x1e)
        {
            WORD val;
            WORD seg;
            WORD offs;
            FLATAMT toamt;
                
            regsize = WORD_SIZE;
            GET_SRC_REG(REG_DS);
            val = FROM_WORD(src);

            GET_SRC_REG(REG_SS);
            seg = FROM_WORD(src);
                
            GET_SRC_REG(REG_SP);
            offs = FROM_WORD(src);

            toamt = GET_FLATAMT(seg, offs);
            toamt -= 2;

            fprintf(logf, "push %s\n", names[REG_DS]);
            dest = src;
            TO_WORD(dest, FROM_WORD(src) - 2);
            WORD_TO_MEM(toamt, val);
            p++;
        }
        /* CB              0023     retf */
        else if (instr == 0xcb)
        {
            WORD seg;
            WORD offs;
            WORD newoffs;
            WORD newseg;
            FLATAMT fromamt;
            FLATAMT newamt;
                
            fprintf(logf, "retf\n");
            
            GET_SRC_REG(REG_SS);
            seg = FROM_WORD(src);
                
            GET_SRC_REG(REG_SP);
            offs = FROM_WORD(src);

            fromamt = GET_FLATAMT(seg, offs);

            newoffs = WORD_AT_MEM(fromamt);
            newseg = WORD_AT_MEM(fromamt + 2);
            
            dest = src;
            TO_WORD(dest, offs + 4);
            
            GET_DEST_REG(REG_CS);
            TO_WORD(dest, newseg);

            newamt = GET_FLATAMT(newseg, newoffs);
            p = base + newamt;
        }
        /* C3              0152      ret */
        else if (instr == 0xc3)
        {
#ifdef PBEMUL
            FLATAMT fromamt;
            FLATAMT newamt;
                
            fprintf(logf, "ret\n");
            
            GET_SRC_REG(REG_ESP);
            fromamt = FROM_DWORD(src);

            newamt = DWORD_AT_MEM(fromamt);
            
            dest = src;
            TO_DWORD(dest, fromamt + sizeof(DWORD));
            
            if (newamt == 0)
            {
                /* a return to 0 is an indicator to terminate */
                break;
            }
            p = base + newamt;
#endif
#ifdef PCEMUL
            WORD seg;
            WORD offs;
            WORD newoffs;
            WORD newseg;
            FLATAMT fromamt;
            FLATAMT newamt;
                
            fprintf(logf, "ret\n");
            
            GET_SRC_REG(REG_SS);
            seg = FROM_WORD(src);
                
            GET_SRC_REG(REG_SP);
            offs = FROM_WORD(src);

            fromamt = GET_FLATAMT(seg, offs);

            newoffs = WORD_AT_MEM(fromamt);
            
            dest = src;
            TO_WORD(dest, offs + 2);
            
            GET_SRC_REG(REG_CS);
            newseg = FROM_WORD(src);

            newamt = GET_FLATAMT(newseg, newoffs);
            p = base + newamt;
#endif
        }
        /* E8B600          0077     call Lba2Chs */
        /* 0130     Lba2Chs proc */
        /* 77 + 3 + b6 = 130 */
        /* E83FFC          03BE           call    _xputfar */
        else if (instr == 0xe8)
        {
            WORD val;
            WORD seg;
            WORD offs;
            WORD newoffs;
            WORD newseg;
            WORD ip;
            FLATAMT toamt;
            FLATAMT newamt;
                
            fprintf(logf, "call 0%02X%02Xh\n", p[2], p[1]);
            
            GET_SRC_REG(REG_SS);
            seg = FROM_WORD(src);
                
            GET_SRC_REG(REG_SP);
            offs = FROM_WORD(src);

            dest = src;
            offs -= 2;
            TO_WORD(dest, offs);
            
            toamt = GET_FLATAMT(seg, offs);

            GET_SRC_REG(REG_CS);
            seg = FROM_WORD(src);
            
            ip = (WORD)(p - base + 3 - ((FLATAMT)seg << 4));
            base[toamt] = ip & 0xff;
            base[toamt + 1] = (ip >> 8) & 0xff;
            
            if (p[2] < 0x80)
            { 
                p += (p[2] << 8) + p[1] + 3;
            }
            else
            {
                p -= 0x10000UL - ((p[2] << 8) + p[1]);
                p += 3;
            }
        }
        /* 0E              0024     push cs */
        else if (instr == 0x0e)
        {
            WORD val;
            WORD seg;
            WORD offs;
            FLATAMT toamt;
                
            regsize = WORD_SIZE;
            GET_SRC_REG(REG_CS);
            val = FROM_WORD(src);

            GET_SRC_REG(REG_SS);
            seg = FROM_WORD(src);
                
            GET_SRC_REG(REG_SP);
            offs = FROM_WORD(src);

            toamt = GET_FLATAMT(seg, offs);
            toamt -= 2;

            fprintf(logf, "push %s\n", names[REG_CS]);
            dest = src;
            TO_WORD(dest, FROM_WORD(src) - 2);
            WORD_TO_MEM(toamt, val);
            p++;
        }
        /* 1F              0025     pop ds */
        else if (instr == 0x1f)
        {
            WORD val;
            WORD seg;
            WORD offs;
            FLATAMT fromamt;
                
            fprintf(logf, "pop %s\n", names[REG_DS]);
            
            regsize = WORD_SIZE;
            GET_SRC_REG(REG_SS);
            seg = FROM_WORD(src);
                
            GET_SRC_REG(REG_SP);
            offs = FROM_WORD(src);

            fromamt = GET_FLATAMT(seg, offs);

            val = WORD_AT_MEM(fromamt);
            
            dest = src;
            TO_WORD(dest, FROM_WORD(src) + 2);

            GET_DEST_REG(REG_DS);
            TO_WORD(dest, val);

            p++;
        }
        /* 07              0178      pop es */
        else if (instr == 0x07)
        {
            WORD val;
            WORD seg;
            WORD offs;
            FLATAMT fromamt;
                
            fprintf(logf, "pop %s\n", names[REG_ES]);
            
            regsize = WORD_SIZE;
            GET_SRC_REG(REG_SS);
            seg = FROM_WORD(src);
                
            GET_SRC_REG(REG_SP);
            offs = FROM_WORD(src);

            fromamt = GET_FLATAMT(seg, offs);

            val = WORD_AT_MEM(fromamt);
            
            dest = src;
            TO_WORD(dest, FROM_WORD(src) + 2);

            GET_DEST_REG(REG_ES);
            TO_WORD(dest, val);

            p++;
        }
        /* 5D pop bp */
        else if (instr == 0x5d)
        {
            WORD val;
            WORD seg;
            WORD offs;
            FLATAMT fromamt;
                
            fprintf(logf, "pop %s\n", names[REG_BP]);
            
            regsize = WORD_SIZE;
            GET_SRC_REG(REG_SS);
            seg = FROM_WORD(src);
                
            GET_SRC_REG(REG_SP);
            offs = FROM_WORD(src);

            fromamt = GET_FLATAMT(seg, offs);

            val = WORD_AT_MEM(fromamt);
            
            dest = src;
            TO_WORD(dest, FROM_WORD(src) + 2);

            GET_DEST_REG(REG_BP);
            TO_WORD(dest, val);

            p++;
        }
        /* 58              017C      pop ax */
        else if (instr == 0x58)
        {
            WORD val;
            WORD seg;
            WORD offs;
            FLATAMT fromamt;
                
            fprintf(logf, "pop %s\n", names[REG_AX]);
            
            regsize = WORD_SIZE;
            GET_SRC_REG(REG_SS);
            seg = FROM_WORD(src);
                
            GET_SRC_REG(REG_SP);
            offs = FROM_WORD(src);

            fromamt = GET_FLATAMT(seg, offs);

            val = WORD_AT_MEM(fromamt);
            
            dest = src;
            TO_WORD(dest, FROM_WORD(src) + 2);

            GET_DEST_REG(REG_AX);
            TO_WORD(dest, val);

            p++;
        }
        /* 59              014C      pop  cx */
        else if (instr == 0x59)
        {
            WORD val;
            WORD seg;
            WORD offs;
            FLATAMT fromamt;
                
            fprintf(logf, "pop %s\n", names[REG_CX]);
            
            regsize = WORD_SIZE;
            GET_SRC_REG(REG_SS);
            seg = FROM_WORD(src);
                
            GET_SRC_REG(REG_SP);
            offs = FROM_WORD(src);

            fromamt = GET_FLATAMT(seg, offs);

            val = WORD_AT_MEM(fromamt);
            
            dest = src;
            TO_WORD(dest, FROM_WORD(src) + 2);

            GET_DEST_REG(REG_CX);
            TO_WORD(dest, val);

            p++;
        }
        /* 5A              014F      pop  dx */
        else if (instr == 0x5a)
        {
            WORD val;
            WORD seg;
            WORD offs;
            FLATAMT fromamt;
                
            fprintf(logf, "pop %s\n", names[REG_DX]);
            
            regsize = WORD_SIZE;
            GET_SRC_REG(REG_SS);
            seg = FROM_WORD(src);
                
            GET_SRC_REG(REG_SP);
            offs = FROM_WORD(src);

            fromamt = GET_FLATAMT(seg, offs);

            val = WORD_AT_MEM(fromamt);
            
            dest = src;
            TO_WORD(dest, FROM_WORD(src) + 2);

            GET_DEST_REG(REG_DX);
            TO_WORD(dest, val);

            p++;
        }
        /* 5B              017B      pop bx */
        else if (instr == 0x5b)
        {
            WORD val;
            WORD seg;
            WORD offs;
            FLATAMT fromamt;
                
            fprintf(logf, "pop %s\n", names[REG_BX]);
            
            regsize = WORD_SIZE;
            GET_SRC_REG(REG_SS);
            seg = FROM_WORD(src);
                
            GET_SRC_REG(REG_SP);
            offs = FROM_WORD(src);

            fromamt = GET_FLATAMT(seg, offs);

            val = WORD_AT_MEM(fromamt);
            
            dest = src;
            TO_WORD(dest, FROM_WORD(src) + 2);

            GET_DEST_REG(REG_BX);
            TO_WORD(dest, val);

            p++;
        }
        /*  5E pop si */
        else if (instr == 0x5e)
        {
            WORD val;
            WORD seg;
            WORD offs;
            FLATAMT fromamt;
            int r;
            
            r = REG_SI;
            fprintf(logf, "pop %s\n", names[r]);
            
            regsize = WORD_SIZE;
            GET_SRC_REG(REG_SS);
            seg = FROM_WORD(src);
                
            GET_SRC_REG(REG_SP);
            offs = FROM_WORD(src);

            fromamt = GET_FLATAMT(seg, offs);

            val = WORD_AT_MEM(fromamt);
            
            dest = src;
            TO_WORD(dest, FROM_WORD(src) + 2);

            GET_DEST_REG(r);
            TO_WORD(dest, val);

            p++;
        }
        /*  5F pop di */
        else if (instr == 0x5f)
        {
            WORD val;
            WORD seg;
            WORD offs;
            FLATAMT fromamt;
            int r;
            
            r = REG_DI;
            fprintf(logf, "pop %s\n", names[r]);
            
            regsize = WORD_SIZE;
            GET_SRC_REG(REG_SS);
            seg = FROM_WORD(src);
                
            GET_SRC_REG(REG_SP);
            offs = FROM_WORD(src);

            fromamt = GET_FLATAMT(seg, offs);

            val = WORD_AT_MEM(fromamt);
            
            dest = src;
            TO_WORD(dest, FROM_WORD(src) + 2);

            GET_DEST_REG(r);
            TO_WORD(dest, val);

            p++;
        }
        /* 8807            mov     [bx],al */
        else if ((instr == 0x88) && (p[1] == 0x07))
        {
            BYTE val;
            WORD seg;
            WORD offs;
            FLATAMT toamt;
            int r;
                
            regsize = BYTE_SIZE;
            r = REG_AL;
            GET_SRC_REG(r);
            val = FROM_BYTE(src);

            GET_SRC_REG(REG_DS);
            seg = FROM_WORD(src);

            GET_SRC_REG(REG_BX);
            offs = FROM_WORD(src);

            fprintf(logf, "mov [%s], %s\n", names[REG_BX], names[r]);

            toamt = GET_FLATAMT(seg, offs);

            BYTE_TO_MEM(toamt, val);
            p += 2;
        }
        /* 8816B501 mov  [BootDisk], dl */
        /* 01B5 is address, offset from ds */
        /* actually 02B5 in mbr.com file because of org 100h */
        else if (instr == 0x88)
        {
            BYTE val;
            WORD seg;
            WORD offs;
            FLATAMT toamt;
                
            regsize = BYTE_SIZE;
            GET_SRC_REG(REG_DL);
            val = FROM_BYTE(src);

            GET_SRC_REG(REG_DS);
            seg = FROM_WORD(src);

            offs = (p[3] << 8) | p[2];

            toamt = GET_FLATAMT(seg, offs);

            if (p[1] != 0x16)
            {
                printf("unknown register %x\n", p[1]);
                exit(EXIT_FAILURE);
            }
            fprintf(logf, "mov [%05Xh], %s\n", toamt, names[REG_DL]);
            
            BYTE_TO_MEM(toamt, val);
            p += 4;
        }
        /* 8A0E1000 mov cl, [FatCount] */
        else if (instr == 0x8a)
        {
            BYTE val;
            WORD seg;
            WORD offs;
            FLATAMT fromamt;
                
            regsize = BYTE_SIZE;
            GET_DEST_REG(REG_CL);

            GET_SRC_REG(REG_DS);
            seg = FROM_WORD(src);

            offs = (p[3] << 8) | p[2];

            fromamt = GET_FLATAMT(seg, offs);
            val = BYTE_AT_MEM(fromamt);

            if (p[1] != 0x0e)
            {
                printf("unknown register %x\n", p[1]);
                exit(EXIT_FAILURE);
            }
            fprintf(logf, "mov %s, [%05Xh]\n", names[REG_CL], fromamt);
            
            TO_BYTE(dest, val);
            p += 4;
        }
#if PBEMUL
        /* 89E5 mov ebp, esp */
        else if ((instr == 0x89)
                 && (p[1] == 0xe5))
        {
            regsize = DWORD_SIZE;
            fprintf(logf, "mov %s,%s\n", names[REG_EBP], names[REG_ESP]);

            GET_DEST_REG(REG_EBP);
            GET_SRC_REG(REG_ESP);
            TO_DWORD(dest, FROM_DWORD(src));
            p += 2;
        }
#endif
        /* 8907            0418           mov     [bx],ax */
        /* 8904            003D     mov [si + 0], ax */
        else if ((instr == 0x89)
                 && ((p[1] == 0x07) || (p[1] == 0x04))
                )
        {
            WORD val;
            WORD seg;
            WORD offs;
            FLATAMT toamt;
            int rl;
            int rr;

            /* right hand first */
            if ((p[1] == 0x07) || (p[1] == 0x04))
            {
                rr = REG_AX;
            }
            else
            {
                printf("unknown register %x\n", p[1]);
                exit(EXIT_FAILURE);
            }
            
            /* left hand */
            if (p[1] == 0x07)
            {
                rl = REG_BX;
            }
            else if (p[1] == 0x04)
            {
                rl = REG_SI;
            }
            else
            {
                printf("unknown register %x\n", p[1]);
                exit(EXIT_FAILURE);
            }
            regsize = WORD_SIZE;

            GET_SRC_REG(rr);
            val = FROM_WORD(src);

            GET_SRC_REG(REG_DS);
            seg = FROM_WORD(src);

            GET_SRC_REG(rl);
            offs = FROM_WORD(src);

            fprintf(logf, "mov [%s], %s\n", names[rl], names[rr]);

            toamt = GET_FLATAMT(seg, offs);

            WORD_TO_MEM(toamt, val);
            p += 2;
        }

        /* C7440C00 00   mov word ptr [si + 12], 0 */
        else if ((instr == 0xc7) && (p[1] == 0x44)) 
        {
            WORD val;
            WORD seg;
            WORD offs;
            FLATAMT toamt;
            int r;

            r = REG_SI;                
            
            regsize = WORD_SIZE;

            val = p[4] << 8 | p[3];

            GET_SRC_REG(REG_DS);
            seg = FROM_WORD(src);

            GET_SRC_REG(r);
            offs = FROM_WORD(src);

            if (p[2] >= 0x80)
            {
                offs -= (0x100 - p[2]);
                fprintf(logf, "mov word ptr [%s-0%02Xh], 0%4Xh\n",
                        names[r], (0x100 - p[2]), val);
            }
            else
            {
                offs += p[2];
                fprintf(logf, "mov word ptr [%s+0%02Xh], 0%4Xh\n",
                        names[r], p[2], val);
            }

            toamt = GET_FLATAMT(seg, offs);

            WORD_TO_MEM(toamt, val);
            p += 5;
        }
        
        /* 8946FE          001C           mov     [bp-2],ax */
        /* 895C02          003F     mov [si + 2], bx */
        /* 894C04          0042     mov [si + 4], cx */
        /* 895406          0045     mov [si + 6], dx */
        /* 897C0A          0048     mov [si + 10], di */
        /* 894408          004C     mov [si + 8], ax */
        else if ((instr == 0x89)
                 && ((p[1] == 0x46) 
                     || (p[1] == 0x5c)
                     || (p[1] == 0x4c)
                     || (p[1] == 0x54)
                     || (p[1] == 0x7c)
                     || (p[1] == 0x44)
                    )
                )
        {
            WORD val;
            WORD seg;
            WORD offs;
            FLATAMT toamt;
            int rr;
            int rl;
                
            /* right */
            if ((p[1] == 0x46) || (p[1] == 0x44))
            {
                rr = REG_AX;
            }
            else if (p[1] == 0x5c)
            {
                rr = REG_BX;
            }
            else if (p[1] == 0x4c)
            {
                rr = REG_CX;
            }
            else if (p[1] == 0x54)
            {
                rr = REG_DX;
            }
            else if (p[1] == 0x7c)
            {
                rr = REG_DI;
            }
            else
            {
                printf("unknown register %x\n", p[1]);
                exit(EXIT_FAILURE);
            }
            
            /* left */
            if (p[1] == 0x46)
            {
                rl = REG_BP;
            }
            else if (p[1] == 0x5c)
            {
                rl = REG_SI;
            }
            else if (p[1] == 0x4c)
            {
                rl = REG_SI;
            }
            else if (p[1] == 0x54)
            {
                rl = REG_SI;
            }
            else if (p[1] == 0x7c)
            {
                rl = REG_SI;
            }
            else if (p[1] == 0x44)
            {
                rl = REG_SI;
            }
            else
            {
                printf("unknown register %x\n", p[1]);
                exit(EXIT_FAILURE);
            }
            
            regsize = WORD_SIZE;

            GET_SRC_REG(rr);
            val = FROM_WORD(src);

            GET_SRC_REG(REG_DS);
            seg = FROM_WORD(src);

            GET_SRC_REG(rl);
            offs = FROM_WORD(src);

            if (p[2] >= 0x80)
            {
                offs -= (0x100 - p[2]);
                fprintf(logf, "mov [%s-0%02Xh], %s\n",
                        names[rl], (0x100 - p[2]), names[rr]);
            }
            else
            {
                offs += p[2];
                fprintf(logf, "mov [%s+0%02Xh], %s\n",
                        names[rl], p[2], names[rr]);
            }

            toamt = GET_FLATAMT(seg, offs);

            WORD_TO_MEM(toamt, val);
            p += 3;
        }
        /* 8986CCFD mov     [bp-564],ax */
        else if ((instr == 0x89) && (p[1] == 0x86))
        {
            WORD val;
            WORD seg;
            WORD offs;
            FLATAMT toamt;
            int rr;
            int rl;
                
            /* right */
            if (p[1] == 0x86)
            {
                rr = REG_AX;
            }
            else
            {
                printf("unknown register %x\n", p[1]);
                exit(EXIT_FAILURE);
            }
            
            /* left */
            if (p[1] == 0x86)
            {
                rl = REG_BP;
            }
            else
            {
                printf("unknown register %x\n", p[1]);
                exit(EXIT_FAILURE);
            }
            
            regsize = WORD_SIZE;

            GET_SRC_REG(rr);
            val = FROM_WORD(src);

            GET_SRC_REG(REG_DS);
            seg = FROM_WORD(src);

            GET_SRC_REG(rl);
            offs = FROM_WORD(src);

            if (p[3] >= 0x80)
            {
                offs -= (0x10000UL - (((unsigned int)p[3] << 8) | p[2]));
                fprintf(logf, "mov [%s-0%04Xh], %s\n",
                        names[rl],
                        (unsigned int)(0x10000UL 
                                       - (((unsigned int)p[3] << 8) | p[2])),
                        names[rr]);
            }
            else
            {
                offs += (((unsigned int)p[3] << 8) | p[2]);
                fprintf(logf, "mov [%s+0%04Xh], %s\n",
                        names[rl],
                        (((unsigned int)p[3] << 8) | p[2]),
                        names[rr]);
            }

            toamt = GET_FLATAMT(seg, offs);

            WORD_TO_MEM(toamt, val);
            p += 4;
        }
        
        /* 8916B301 mov [Heads], dx */
        /* 890EB101 mov [SectorsPerTrack], cx */
        /* 01B1 is target */
        else if (instr == 0x89)
        {
            WORD val;
            WORD seg;
            WORD offs;
            FLATAMT toamt;
            int r;
                
            regsize = WORD_SIZE;
            if (p[1] == 0x16)
            {
                r = REG_DX;
            }
            else
            {
                r = REG_CX;
            }
            GET_SRC_REG(r);
            val = FROM_WORD(src);

            GET_SRC_REG(REG_DS);
            seg = FROM_WORD(src);

            offs = (p[3] << 8) | p[2];

            toamt = GET_FLATAMT(seg, offs);

            if ((p[1] != 0x16) && (p[1] != 0x0e))
            {
                printf("unknown register %x\n", p[1]);
                exit(EXIT_FAILURE);
            }
            fprintf(logf, "mov [%05Xh], %s\n", toamt, names[r]);
            
            BYTE_TO_MEM(toamt, val);
            p += 4;
        }
        /* 8BEE            00A0     mov bp,si */
        else if ((instr == 0x8b) && (p[1] == 0xee))
        {
            regsize = WORD_SIZE;
            fprintf(logf, "mov %s,%s\n", names[REG_BP], names[REG_SI]);

            GET_DEST_REG(REG_BP);
            GET_SRC_REG(REG_SI);
            TO_WORD(dest, FROM_WORD(src));
            
            p += 2;
        }
        /* 8BDA            001E     mov bx, dx */
        else if ((instr == 0x8b) && (p[1] == 0xda))
        {
            regsize = WORD_SIZE;
            fprintf(logf, "mov %s,%s\n", names[REG_BX], names[REG_DX]);

            GET_DEST_REG(REG_BX);
            GET_SRC_REG(REG_DX);
            TO_WORD(dest, FROM_WORD(src));
            
            p += 2;
        }
        /* 8BC2            071F           mov     ax,dx */
        else if ((instr == 0x8b) && (p[1] == 0xc2))
        {
            regsize = WORD_SIZE;
            fprintf(logf, "mov %s,%s\n", names[REG_AX], names[REG_DX]);

            GET_DEST_REG(REG_AX);
            GET_SRC_REG(REG_DX);
            TO_WORD(dest, FROM_WORD(src));
            
            p += 2;
        }
        /* 8BD8            00A9           mov     bx,ax */
        else if ((instr == 0x8b) && (p[1] == 0xd8))
        {
            regsize = WORD_SIZE;
            fprintf(logf, "mov %s,%s\n", names[REG_BX], names[REG_AX]);

            GET_DEST_REG(REG_BX);
            GET_SRC_REG(REG_AX);
            TO_WORD(dest, FROM_WORD(src));
            
            p += 2;
        }
        /* 8BEC            0001           mov     bp,sp */
        else if ((instr == 0x8b) && (p[1] == 0xec))
        {
            regsize = WORD_SIZE;
            fprintf(logf, "mov %s,%s\n", names[REG_BP], names[REG_SP]);

            GET_DEST_REG(REG_BP);
            GET_SRC_REG(REG_SP);

            TO_WORD(dest, FROM_WORD(src));
            
            p += 2;
        }
        /* 8B07            0586           mov     ax,[bx] */
        else if ((instr == 0x8b) && (p[1] == 0x07))
        {
            WORD seg;
            WORD offs;
            FLATAMT fromamt;
            int r;
            int r2;
                
            regsize = WORD_SIZE;
            if (p[1] == 0x07)
            {
                r = REG_BX;
                r2 = REG_AX;
            }
            else
            {
                printf("unknown register %x\n", p[1]);
                exit(EXIT_FAILURE);
            }
            GET_SRC_REG(r);
            offs = FROM_WORD(src);

            GET_SRC_REG(REG_DS);
            seg = FROM_WORD(src);

            fromamt = GET_FLATAMT(seg, offs);
            
            fprintf(logf, "mov %s, [%s]\n", names[r2], names[r]);
            
            GET_DEST_REG(r2);
            TO_WORD(dest, WORD_AT_MEM(fromamt));
            
            p += 2;
        }
        /* 8B04            0018     mov ax, word ptr [si + 0] */
        else if ((instr == 0x8b) && (p[1] == 0x04))
        {
            WORD seg;
            WORD offs;
            FLATAMT fromamt;
            int r;
            int r2;
                
            regsize = WORD_SIZE;
            
            /* check for right-hand register */
            if (p[1] == 0x04)
            {
                r = REG_SI;
            }
            else
            {
                printf("unknown register %x\n", p[1]);
                exit(EXIT_FAILURE);
            }
            
            /* check for left hand */
            if (p[1] == 0x04)
            {
                r2 = REG_AX;
            }
            
            GET_SRC_REG(r);
            offs = FROM_WORD(src);

            GET_SRC_REG(REG_DS);
            seg = FROM_WORD(src);

            fromamt = GET_FLATAMT(seg, offs);

            fprintf(logf, "mov %s, [%s+0h]\n", names[r2], names[r]);
            
            GET_DEST_REG(r2);
            TO_WORD(dest, WORD_AT_MEM(fromamt));
            
            p += 2;
        }
        
        /* 8B86CCFD mov     ax,[bp-564] */
        else if ((instr == 0x8b) && (p[1] == 0x86))
        {
            WORD seg;
            WORD offs;
            FLATAMT fromamt;
            int r;
            int r2;
                
            regsize = WORD_SIZE;
            
            /* check for right-hand register */
            if (p[1] == 0x86)
            {
                r = REG_BP;
            }
            else
            {
                printf("unknown register %x\n", p[1]);
                exit(EXIT_FAILURE);
            }
            
            /* check for left hand */
            if (p[1] == 0x86)
            {
                r2 = REG_AX;
            }
            else
            {
                printf("unknown register %x\n", p[1]);
                exit(EXIT_FAILURE);
            }
            
            GET_SRC_REG(r);
            offs = FROM_WORD(src);

            if (r == REG_BP)
            {
                GET_SRC_REG(REG_SS);
            }
            else
            {
                GET_SRC_REG(REG_DS);
            }
            seg = FROM_WORD(src);

            fromamt = GET_FLATAMT(seg, offs);

            if (p[3] >= 0x80)
            {
                fromamt -= (0x10000UL - (((unsigned int)p[3] << 8) | p[2]));
                fprintf(logf, "mov %s, [%s-0%04Xh]\n",
                        names[r2],
                        names[r],
                        (0x10000UL - (((unsigned int)p[3] << 8) | p[2])));
            }
            else
            {
                fromamt += (((unsigned int)p[3] << 8) | p[2]);
                fprintf(logf, "mov %s, [%s+0%02Xh]\n",
                        names[r2],
                        names[r],
                        (((unsigned int)p[3] << 8) | p[2]));
            }
            
            GET_DEST_REG(r2);
            TO_WORD(dest, WORD_AT_MEM(fromamt));
            
            p += 4;
        }


#if PBEMUL
        /* FF5034 call [eax+52] */
        else if ((instr == 0xff)
                 && (p[1] == 0x50))
        {
            FLATAMT fromamt;
            int r;
            DWORD val;
                
            regsize = DWORD_SIZE;
            
            if (p[1] == 0x50)
            {
                r = REG_EAX;
            }
            else
            {
                printf("unknown register %x\n", p[1]);
                exit(EXIT_FAILURE);
            }
            
            GET_SRC_REG(r);

            fromamt = FROM_DWORD(src);

            if (p[2] >= 0x80)
            {
                fromamt -= (0x100 - p[2]);
                fprintf(logf, "call [%s-0%02Xh]\n",
                        names[r], (0x100 - p[2]));
            }
            else
            {
                fromamt += p[2];
                fprintf(logf, "call [%s+0%02Xh]\n",
                        names[r], p[2]);
            }
            
            val = DWORD_AT_MEM(fromamt);
            
            if (val < 1000)
            {
                esp -= sizeof(DWORD);
                spec_call(val);
                esp += sizeof(DWORD);
            }
            else
            {
                printf("normal call not yet implemented\n");
                exit(0);
            }
            
            p += 3;
        }
#endif

#if PBEMUL
        /* FF7020 push [eax+32] */
        else if (instr == 0xff)
        {
            FLATAMT fromamt;
            FLATAMT toamt;
            int r;
            int r2;
            DWORD val;
                
            regsize = DWORD_SIZE;
            
            /* check for right-hand register */
            if (p[1] == 0x70)
            {
                r = REG_EAX;
            }
            else
            {
                printf("unknown register %x\n", p[1]);
                exit(EXIT_FAILURE);
            }
            
            GET_SRC_REG(r);

            r2 = REG_ESP;

            fromamt = FROM_DWORD(src);

            if (p[2] >= 0x80)
            {
                fromamt -= (0x100 - p[2]);
                fprintf(logf, "push [%s-0%02Xh]\n",
                        names[r], (0x100 - p[2]));
            }
            else
            {
                fromamt += p[2];
                fprintf(logf, "push [%s+0%02Xh]\n",
                        names[r], p[2]);
            }
            
            val = DWORD_AT_MEM(fromamt);
            
            GET_SRC_REG(REG_ESP);
            toamt = FROM_DWORD(src);
            toamt -= sizeof(DWORD);
            
            dest = src;
            TO_DWORD(dest, FROM_DWORD(src) - sizeof(DWORD));
            DWORD_TO_MEM(toamt, val);
                        
            p += 3;
        }
#endif

#if PBEMUL
        /* 8B4508 mov eax,[ebp+8] */
        else if (instr == 0x8b)
        {
            FLATAMT fromamt;
            int r;
            int r2;
                
            regsize = DWORD_SIZE;
            
            /* check for right-hand register */
            if (p[1] == 0x45)
            {
                r = REG_EBP;
            }
            else
            {
                printf("unknown register %x\n", p[1]);
                exit(EXIT_FAILURE);
            }
            
            /* check for left hand */
            if (p[1] == 0x45)
            {
                r2 = REG_EAX;
            }
            else
            {
                printf("unknown register %x\n", p[1]);
                exit(EXIT_FAILURE);
            }
            
            GET_SRC_REG(r);

            fromamt = FROM_DWORD(src);

            if (p[2] >= 0x80)
            {
                fromamt -= (0x100 - p[2]);
                fprintf(logf, "mov %s, [%s-0%02Xh]\n",
                        names[r2], names[r], (0x100 - p[2]));
            }
            else
            {
                fromamt += p[2];
                fprintf(logf, "mov %s, [%s+0%02Xh]\n",
                        names[r2], names[r], p[2]);
            }
            
            GET_DEST_REG(r2);
            TO_DWORD(dest, DWORD_AT_MEM(fromamt));
            
            p += 3;
        }
#endif

#if PCEMUL
        /* 8B4408          0071     mov ax,[si+8] */
        /* 8B540A          0074     mov dx,[si+10] */
        /* 8B4608          0013           mov     ax,[bp+8] */
        /* 8B7608          000C       mov si, sregs (bp+??) */
        /* 8B5C02          001A     mov bx, word ptr [si + 2] */
        /* 8B4C04          001D     mov cx, word ptr [si + 4] */
        /* 8B7C0A          0023     mov di, word ptr [si + 10] */
        /* 8B7408          0026     mov si, word ptr [si + 8] */
        else if (instr == 0x8b)
        {
            WORD seg;
            WORD offs;
            FLATAMT fromamt;
            int r;
            int r2;
                
            regsize = WORD_SIZE;
            
            /* probably last 3 bits are what matters */
            /* 4 = SI, 6 = BP */
            /* check for right-hand register */
            if ((p[1] == 0x44) || (p[1] == 0x54) || (p[1] == 0x5c)
                || (p[1] == 0x4c) || (p[1] == 0x7c) || (p[1] == 0x78)
                || (p[1] == 0x74))
            {
                r = REG_SI;
            }
            else if ((p[1] == 0x46) || (p[1] == 0x76))
            {
                r = REG_BP;
            }
            else
            {
                printf("unknown register %x\n", p[1]);
                exit(EXIT_FAILURE);
            }
            
            /* probably 3 bits before that are what matters */
            /* or maybe 4 - otherwise what is the first 4 in 76? */
            /* 4-bit theory - 58 (all >> 3) = 1011 = BX
                              50                   = DX
                              70                   = SI
                              40                   = AX
                              48                   = CX
                              78                   = DI
                              70                   = SI
            */
            /* check for left hand */
            if ((p[1] == 0x44) || (p[1] == 0x46))
            {
                r2 = REG_AX;
            }
            else if (p[1] == 0x54)
            {
                r2 = REG_DX;
            }
            else if (p[1] == 0x76)
            {
                r2 = REG_SI;
            }
            else if (p[1] == 0x5c)
            {
                r2 = REG_BX;
            }
            else if (p[1] == 0x4c)
            {
                r2 = REG_CX;
            }
            else if (p[1] == 0x7c)
            {
                r2 = REG_DI;
            }
            else if (p[1] == 0x74)
            {
                r2 = REG_SI;
            }
            else
            {
                printf("unknown register %x\n", p[1]);
                exit(EXIT_FAILURE);
            }
            
            GET_SRC_REG(r);
            offs = FROM_WORD(src);

            if (r == REG_BP)
            {
                GET_SRC_REG(REG_SS);
            }
            else
            {
                GET_SRC_REG(REG_DS);
            }
            seg = FROM_WORD(src);

            fromamt = GET_FLATAMT(seg, offs);

            if (p[2] >= 0x80)
            {
                fromamt -= (0x100 - p[2]);
                fprintf(logf, "mov %s, [%s-0%02Xh]\n",
                        names[r2], names[r], (0x100 - p[2]));
            }
            else
            {
                fromamt += p[2];
                fprintf(logf, "mov %s, [%s+0%02Xh]\n",
                        names[r2], names[r], p[2]);
            }
            
            GET_DEST_REG(r2);
            TO_WORD(dest, WORD_AT_MEM(fromamt));
            
            p += 3;
        }
#endif

        /* F60480          0030     test byte ptr [si],080h */
        else if (instr == 0xf6)
        {
            BYTE val;
            WORD seg;
            WORD offs;
            FLATAMT fromamt;
                
            if (p[1] != 0x04)
            {
                printf("unknown register %x\n", p[1]);
                exit(EXIT_FAILURE);
            }
            
            regsize = WORD_SIZE;
            GET_SRC_REG(REG_SI);
            offs = FROM_WORD(src);

            GET_SRC_REG(REG_DS);
            seg = FROM_WORD(src);

            fromamt = GET_FLATAMT(seg, offs);

            val = BYTE_AT_MEM(fromamt);
            
            fprintf(logf, "test byte ptr [si], 0%02Xh\n", p[2]);
            
            val &= p[2];
            
            zero = (val == 0);
            
            p += 3;
        }
        /* 750A            0033     jnz found_active */
        else if (instr == 0x75)
        {
            BYTE val;
            WORD seg;
            WORD offs;
            FLATAMT fromamt;
            int dir;

            if (p[1] >= 0x80)
            {
                dir = -(0x100 - p[1]);
            }
            else
            {
                dir = p[1];
            }
            fprintf(logf, "jnz 0%05lXh\n", (unsigned long)((p - base) + 2 + dir));

            if (!zero)
            {
                p = p + 2 + dir;
            }
            else
            {
                p += 2; /* this is a 2-byte instruction */
            }
        }
        /* EB09            007A     jmp donemanual */
        else if (instr == 0xeb)
        {
            BYTE val;
            WORD seg;
            WORD offs;
            FLATAMT fromamt;
            int dir;

            if (p[1] >= 0x80)
            {
                dir = -(0x100 - p[1]);
            }
            else
            {
                dir = p[1];
            }
            fprintf(logf, "jmp 0%05lXh\n", (unsigned long)((p - base) + 2 + dir));

            p = p + 2 + dir;
        }
        /* E9F901          0105           jmp     L11 */
        else if (instr == 0xe9)
        {
            WORD val;
            WORD seg;
            WORD offs;
            FLATAMT fromamt;
            int dir;

            if (p[2] >= 0x80)
            {
                dir = (int)-(0x10000UL - (((unsigned int)p[2] << 8) | p[1]));
            }
            else
            {
                dir = ((unsigned int)p[2] << 8) | p[1];
            }
            fprintf(logf, "jmp 0%05lXh\n", (unsigned long)((p - base) + 3 + dir));

            p = p + 3 + dir;
        }
        /* 807C0400 cmp byte ptr [si+4],0; */
        /* don't confuse with 80E13F   and cl,3fh */
        else if ((instr == 0x80) && (p[1] == 0x7c))
        {
            BYTE val;
            WORD seg;
            WORD offs;
            unsigned int extra;
            FLATAMT fromamt;
                
            if (p[1] != 0x7c)
            {
                printf("unknown register %x\n", p[1]);
                exit(EXIT_FAILURE);
            }
            
            regsize = WORD_SIZE;
            GET_SRC_REG(REG_SI);
            offs = FROM_WORD(src);

            GET_SRC_REG(REG_DS);
            seg = FROM_WORD(src);

            fromamt = GET_FLATAMT(seg, offs);
            
            extra = p[2]; /* is this signed? */
            fromamt += extra;

            val = BYTE_AT_MEM(fromamt);
            
            fprintf(logf, "cmp byte ptr [si+0%04Xh], 0%02Xh\n", extra, p[3]);
            
            /* are these the right way around? */
            zero = 0;
            if (val == p[3])
            {
                lt = 0;
                gt = 0;
                eq = 1;
                if (val == 0)
                {
                    zero = 1; /* just a guess */
                }
            }
            else if (val < p[3])
            {
                lt = 1;
                gt = 0;
                eq = 0;
            }
            else
            {
                lt = 0;
                gt = 1;
                eq = 0;
            }
            
            p += 4;
        }
        /* 80FA80          007A     cmp dl, 080h */
        else if ((instr == 0x80) && (p[1] == 0xfa))
        {
            BYTE val;
            BYTE val2;

            regsize = BYTE_SIZE;
            GET_SRC_REG(REG_DL);
            val = p[2];
            fprintf(logf, "cmp %s,0%02Xh\n", names[REG_DL], (unsigned int)val);
            val2 = FROM_BYTE(src);
            zero = 0;
            /* is this the right way around? */
            if (val == val2)
            {
                lt = 0;
                gt = 0;
                eq = 1;
                zero = 1; /* it seems that equal also sets zero */
            }
            else if (val < val2)
            {
                lt = 1;
                gt = 0;
                eq = 0;
            }
            else
            {
                lt = 0;
                gt = 1;
                eq = 0;
            }
            p += 3;
        }
        /* 80E13F   and cl,3fh */
        else if (instr == 0x80)
        {
            BYTE val;
            BYTE val2;

            if (p[1] != 0xe1)
            {
                printf("unknown register %x\n", p[1]);
                exit(EXIT_FAILURE);
            }
            
            regsize = BYTE_SIZE;
            GET_SRC_REG(REG_CL);
            val2 = FROM_BYTE(src);
            dest = src;
            val = p[2];
            fprintf(logf, "and %s,0%02Xh\n", names[REG_CL], (unsigned int)val);
            TO_BYTE(dest, val2 & val);
            p += 3;
        }
        /* 2500FF          0046      and ax, 0ff00h */
        else if (instr == 0x25)
        {
            WORD val;
            WORD val2;

            regsize = WORD_SIZE;
            GET_SRC_REG(REG_AX);
            val2 = FROM_WORD(src);
            dest = src;
            val = (p[2] << 8) | p[1];
            fprintf(logf, "and %s,0%04Xh\n", names[REG_AX], (unsigned int)val);
            TO_WORD(dest, val2 & val);
            p += 3;
        }
        /* 21C8            0846           and     ax,cx */
        else if (instr == 0x21)
        {
            WORD val;
            WORD val2;

            if (p[1] != 0xc8)
            {
                printf("unknown register %x\n", p[1]);
                exit(EXIT_FAILURE);
            }
            regsize = WORD_SIZE;
            GET_SRC_REG(REG_CX);
            val = FROM_WORD(src);
            GET_SRC_REG(REG_AX);
            val2 = FROM_WORD(src);
            dest = src;
            fprintf(logf, "and %s,%s\n", names[REG_AX], names[REG_CX]);
            TO_WORD(dest, val2 & val);
            p += 2;
        }
        /* FEC1            0138      inc  cl */
        else if (instr == 0xfe)
        {
            BYTE val;

            if (p[1] != 0xc1)
            {
                printf("unknown register %x\n", p[1]);
                exit(EXIT_FAILURE);
            }
            
            regsize = BYTE_SIZE;
            GET_SRC_REG(REG_CL);
            val = FROM_BYTE(src);
            val++;
            dest = src;
            fprintf(logf, "inc cl\n");
            TO_BYTE(dest, val);
            p += 2;
        }
        /* 45              00AD     inc bp */
        else if (instr == 0x45)
        {
            BYTE val;

            regsize = BYTE_SIZE;
            GET_SRC_REG(REG_BP);
            val = FROM_BYTE(src);
            val++;
            dest = src;
            fprintf(logf, "inc bp\n");
            TO_BYTE(dest, val);
            p++;
        }

#if PBEMUL
        /* 83EC08 sub esp, $8 */
        else if ((instr == 0x83)
                 && (p[1] == 0xec))
        {
            BYTE val;
            DWORD val2;
            int r;

            if (p[1] == 0xec)
            {
                r = REG_ESP;
            }
            else
            {
                printf("unknown register %x\n", p[1]);
                exit(EXIT_FAILURE);
            }
            
            regsize = DWORD_SIZE;
            GET_SRC_REG(r);
            val2 = FROM_DWORD(src);
            dest = src;
            val = p[2];
            val2 -= val;
            fprintf(logf, "sub %s,0%02Xh\n", names[r], (unsigned int)val);
            TO_DWORD(dest, val2);
            p += 3;
        }
#endif

        /* 83C201          006A      add dx,1 */
        /* 83C001          014B       add  ax,1 */
        /* 83C4FE          0010           add     sp,-2 */
        else if ((instr == 0x83)
                 && ((p[1] == 0xc2)
                     || (p[1] == 0xc0)
                     || (p[1] == 0xc4)
                    )
                )
        {
            BYTE val;
            WORD val2;
            WORD oldval2;
            int r;

            if (p[1] == 0xc2)
            {
                r = REG_DX;
            }
            else if (p[1] == 0xc0)
            {
                r = REG_AX;
            }
            else if (p[1] == 0xc4)
            {
                r = REG_SP;
            }
            else
            {
                printf("unknown register %x\n", p[1]);
                exit(EXIT_FAILURE);
            }
            
            regsize = WORD_SIZE;
            GET_SRC_REG(r);
            val2 = FROM_WORD(src);
            dest = src;
            val = p[2];
            oldval2 = val2;
            if (val >= 0x80)
            {
                val2 -= (0x100 - val);
                if (val2 > oldval2)
                {
                    cflag = 1;
                }
                else
                {
                    cflag = 0;
                }
            }
            else
            {
                val2 += val;
                if (val2 < oldval2)
                {
                    cflag = 1;
                }
                else
                {
                    cflag = 0;
                }
            }
            fprintf(logf, "add %s,0%02Xh\n", names[r], (unsigned int)val);
            TO_WORD(dest, val2);
            p += 3;
        }
        /* 83D200          014E       adc  dx,0 */
        else if ((instr == 0x83) && (p[1] == 0xd2))
        {
            BYTE val;
            WORD val2;
            WORD oldval2;
            int r;

            if (p[1] == 0xd2)
            {
                r = REG_DX;
            }
            else
            {
                printf("unknown register %x\n", p[1]);
                exit(EXIT_FAILURE);
            }
            
            regsize = WORD_SIZE;
            GET_SRC_REG(r);
            val2 = FROM_WORD(src);
            dest = src;
            val = p[2];
            oldval2 = val2;
            val2 += (val + cflag);
            if (val2 < oldval2)
            {
                cflag = 1;
            }
            else
            {
                cflag = 0;
            }
            fprintf(logf, "adc %s,0%02Xh\n", names[r], (unsigned int)val);
            TO_WORD(dest, val2);
            p += 3;
        }
        /* B600            0068      mov dh,0 */
        else if (instr == 0xb6)
        {
            BYTE val;

            regsize = BYTE_SIZE;
            GET_DEST_REG(REG_DH);
            val = p[1];
            fprintf(logf, "mov %s,0%02Xh\n", names[REG_DH], (unsigned int)val);
            TO_BYTE(dest, val);
            p += 2;
        }
        /* B000            0145      mov  al, 0 */
        else if (instr == 0xb0)
        {
            BYTE val;

            regsize = BYTE_SIZE;
            GET_DEST_REG(REG_AL);
            val = p[1];
            fprintf(logf, "mov %s,0%02Xh\n", names[REG_AL], (unsigned int)val);
            TO_BYTE(dest, val);
            p += 2;
        }
        /* B102            0148      mov  cl, 2 */
        else if (instr == 0xb1)
        {
            BYTE val;

            regsize = BYTE_SIZE;
            GET_DEST_REG(REG_CL);
            val = p[1];
            fprintf(logf, "mov %s,0%02Xh\n", names[REG_CL], (unsigned int)val);
            TO_BYTE(dest, val);
            p += 2;
        }
        /* BDD800          0043     mov bp,offset active_partition_invalid */
        else if (instr == 0xbd)
        {
            WORD val;

            regsize = WORD_SIZE;
            GET_DEST_REG(REG_BP);
            val = p[2] << 8 | p[1];
            fprintf(logf, "mov %s,0%04Xh\n", names[REG_BP], (int)val);
            TO_WORD(dest, val);
            p += 3;
        }
        /* 7466            0046     jz fatal_error */
        else if (instr == 0x74)
        {
            BYTE val;
            WORD seg;
            WORD offs;
            FLATAMT fromamt;
            int dir;

            if (p[1] >= 0x80)
            {
                dir = -(0x100 - p[1]);
            }
            else
            {
                dir = p[1];
            }
            fprintf(logf, "jz 0%05lXh\n", (unsigned long)((p - base) + 2 + dir));

            if (zero)
            {
                p = p + 2 + dir;
            }
            else
            {
                p += 2; /* this is a 2-byte instruction */
            }
        }
        /* 26C706FE 7D0000    mov word ptr es:[07dfeh],0h */
        else if ((instr == 0x26) && (p[1] == 0xc7))
        {
            WORD val;
            WORD seg;
            WORD offs;
            FLATAMT toamt;
                
            regsize = WORD_SIZE;
            
            val = (p[6] << 8) + p[5];
            
            GET_SRC_REG(REG_ES);
            seg = FROM_WORD(src);

            offs = (p[4] << 8) | p[3];

            toamt = GET_FLATAMT(seg, offs);

            if (p[1] != 0xc7)
            {
                printf("unknown register %x\n", p[1]);
                exit(EXIT_FAILURE);
            }
            if (p[2] != 0x06)
            {
                printf("unknown register %x\n", p[2]);
                exit(EXIT_FAILURE);
            }
            fprintf(logf, "mov word ptr es:[%04Xh], 0%04Xh\n", offs, val);
            
            WORD_TO_MEM(toamt, val);
            p += 7;
        }
        /* 837E0408 cmp intnum, 08h */
        /* this is 7E for BP apparently, offset 4 */
        /* and it is a word comparison even though the
           comparator is just 1 byte */
        else if (instr == 0x83)
        {
            BYTE val;
            WORD val2;
            WORD seg;
            WORD offs;
            FLATAMT fromamt;
                
            if (p[1] != 0x7e)
            {
                printf("unknown register %x\n", p[1]);
                exit(EXIT_FAILURE);
            }
            
            regsize = WORD_SIZE;
            
            val = p[3];
            
            GET_SRC_REG(REG_SS);
            seg = FROM_WORD(src);

            GET_SRC_REG(REG_BP);
            offs = FROM_WORD(src);
            
            if (p[2] >= 0x80)
            {
                printf("not sure if this is signed\n");
                exit(EXIT_FAILURE);
            }
            
            offs += p[2];

            fromamt = GET_FLATAMT(seg, offs);

            fprintf(logf, "cmp word ptr [bp+0%02Xh], 0%02Xh\n", p[2], val);
            
            val2 = WORD_AT_MEM(fromamt);
            
            zero = 0;
            /* is this the right way around? */
            if (val == val2)
            {
                lt = 0;
                gt = 0;
                eq = 1;
                zero = 1; /* it seems that equal also sets zero */
            }
            else if (val < val2)
            {
                lt = 1;
                gt = 0;
                eq = 0;
            }
            else
            {
                lt = 0;
                gt = 1;
                eq = 0;
            }
            
            p += 4;
        }
        /* 26813EFE  7D55AA   cmp word ptr es:[07dfeh],0aa55h */
        else if ((instr == 0x26) && (p[1] == 0x81))
        {
            WORD val;
            WORD val2;
            WORD seg;
            WORD offs;
            FLATAMT fromamt;
                
            regsize = WORD_SIZE;
            
            val = (p[6] << 8) + p[5];
            
            GET_SRC_REG(REG_ES);
            seg = FROM_WORD(src);

            offs = (p[4] << 8) | p[3];

            fromamt = GET_FLATAMT(seg, offs);

            if (p[1] != 0x81)
            {
                printf("unknown register %x\n", p[1]);
                exit(EXIT_FAILURE);
            }
            if (p[2] != 0x3e)
            {
                printf("unknown register %x\n", p[2]);
                exit(EXIT_FAILURE);
            }
            fprintf(logf, "cmp word ptr es:[%04Xh], 0%04Xh\n", offs, val);
            
            val2 = WORD_AT_MEM(fromamt);
            
            zero = 0;
            /* is this the right way around? */
            if (val == val2)
            {
                lt = 0;
                gt = 0;
                eq = 1;
                zero = 1; /* it seems that equal also sets zero */
            }
            else if (val < val2)
            {
                lt = 1;
                gt = 0;
                eq = 0;
            }
            else
            {
                lt = 0;
                gt = 1;
                eq = 0;
            }
            
            p += 7;
        }
        /* 26881602 7C mov es:[07C02h],dl */
        else if ((instr == 0x26) && (p[1] == 0x88))
        {
            BYTE val;
            BYTE val2;
            WORD seg;
            WORD offs;
            FLATAMT fromamt;
                
            regsize = BYTE_SIZE;
            
            GET_SRC_REG(REG_DL);
            val = FROM_BYTE(src);

            GET_SRC_REG(REG_ES);
            seg = FROM_WORD(src);

            offs = (p[4] << 8) | p[3];

            fromamt = GET_FLATAMT(seg, offs);

            if (p[1] != 0x88)
            {
                printf("unknown register %x\n", p[1]);
                exit(EXIT_FAILURE);
            }
            if (p[2] != 0x16)
            {
                printf("unknown register %x\n", p[2]);
                exit(EXIT_FAILURE);
            }
            fprintf(logf, "mov es:[0%04Xh], dl\n", offs);
            
            val2 = BYTE_AT_MEM(fromamt);

            zero = 0;
            /* is this the right way around? */
            if (val == val2)
            {
                lt = 0;
                gt = 0;
                eq = 1;
                zero = 1; /* it seems that equal also sets zero */
            }
            else if (val < val2)
            {
                lt = 1;
                gt = 0;
                eq = 0;
            }
            else
            {
                lt = 0;
                gt = 1;
                eq = 0;
            }
            
            p += 5;
        }
        /* 26881602 7C  mov es:[07C02h],dl */
        /* F736B101 div  word ptr [SectorsPerTrack] */
        else if ((instr == 0xf7) && (p[1] == 0x36))
        {
            WORD val;
            WORD seg;
            WORD offs;
            FLATAMT fromamt;
            DWORD pair;
                
            regsize = WORD_SIZE;
            
            val = (p[6] << 8) + p[5];
            
            GET_SRC_REG(REG_DS);
            seg = FROM_WORD(src);

            offs = (p[3] << 8) | p[2];

            fromamt = GET_FLATAMT(seg, offs);
            val = WORD_AT_MEM(fromamt);

            if (p[1] != 0x36)
            {
                printf("unknown register %x\n", p[1]);
                exit(EXIT_FAILURE);
            }
            fprintf(logf, "div word ptr [0%04Xh]\n", offs);
            
            GET_SRC_REG(REG_DX);
            pair = FROM_WORD(src);
            GET_SRC_REG(REG_AX);
            pair = (pair << 16) + FROM_WORD(src);

            /* testing says these are the right way around */            
            GET_DEST_REG(REG_AX);
            TO_WORD(dest, pair / val);

            GET_DEST_REG(REG_DX);
            TO_WORD(dest, pair % val);
            
            p += 4;
        }
        /* F7E1            00C6      mul cx */
        /* F7E9            05B4           imul    cx */
        /* F7F9            0638           idiv    cx */        
        else if (instr == 0xf7)
        {
            WORD val;
            WORD seg;
            WORD offs;
            FLATAMT fromamt;
            DWORD pair;
            int i = 0; /* this says whether it is imul - presumably
                      signed multiply */
            int div = 0;
                
            regsize = WORD_SIZE;
            
            if (p[1] == 0xe1)
            {
                /* nothing */
            }
            else if (p[1] == 0xe9)
            {
                i = 1;
            }
            else if (p[1] == 0xf9)
            {
                i = 1;
                div = 1;
            }
            else
            {
                printf("unknown register %x\n", p[1]);
                exit(EXIT_FAILURE);
            }
            if (i)
            {
                if (div)
                {
                    /* i don't think we need idiv */
                    fprintf(logf, "idiv cx - changed to div\n");
                }
                else
                {
                    /* i dont think we need imul */
                    fprintf(logf, "imul cx - changed to mul\n");
                }
            }
            else
            {
                fprintf(logf, "mul cx\n");
            }
            
            GET_SRC_REG(REG_DX);
            pair = FROM_WORD(src);
            GET_SRC_REG(REG_AX);
            pair = (pair << 16) + FROM_WORD(src);

            GET_SRC_REG(REG_CX);
            val = FROM_WORD(src);
            
            if (div)
            {
                DWORD temppair;
                
                if (val == 0)
                {
                    printf("divide by zero\n");
                    exit(EXIT_FAILURE);
                }
                /* are these the right way around? */
                temppair = pair;
                pair %= val;
                temppair /= val;
                pair <<= 16;
                pair |= temppair & 0xffffU;
            }
            else
            {
                pair *= val;
            }
            
            /* guess this is the right way around */
            GET_DEST_REG(REG_AX);
            TO_WORD(dest, pair & 0xffffU);

            GET_DEST_REG(REG_DX);
            TO_WORD(dest, (pair >> 16) & 0xffffU);
            
            p += 2;
        }
        /* B408            0054      mov ah, 08h */
        else if (instr == 0xb4)
        {
            BYTE val;

            regsize = BYTE_SIZE;
            GET_DEST_REG(REG_AH);
            val = p[1];
            fprintf(logf, "mov %s,0%02Xh\n", names[REG_AH], (unsigned int)val);
            TO_BYTE(dest, val);
            p += 2;
        }
        /* B500            0060      mov ch, 0 */
        else if (instr == 0xb5)
        {
            BYTE val;

            regsize = BYTE_SIZE;
            GET_DEST_REG(REG_CH);
            val = p[1];
            fprintf(logf, "mov %s,0%02Xh\n", names[REG_CH], (unsigned int)val);
            TO_BYTE(dest, val);
            p += 2;
        }
        /* CD13            0059      int 13h */
        else if (instr == 0xcd)
        {
            if ((p[1] == 0x13) || (p[1] == 0x10))
            {
                dobios(p[1]);
            }
            else
            {
                printf("unknown interrupt %x\n", p[1]);
                exit(EXIT_FAILURE);
            }
            p += 2;
        }
        /* 721F            005B      jc ignorec */
        else if (instr == 0x72)
        {
            BYTE val;
            WORD seg;
            WORD offs;
            FLATAMT fromamt;
            int dir;

            if (p[1] >= 0x80)
            {
                dir = -(0x100 - p[1]);
            }
            else
            {
                dir = p[1];
            }
            fprintf(logf, "jc 0%05lXh\n", (unsigned long)((p - base) + 2 + dir));

            if (cflag)
            {
                p = p + 2 + dir;
            }
            else
            {
                p += 2; /* this is a 2-byte instruction */
            }
        }
        /* 7C02            0070           jl      L8 */
        else if (instr == 0x7c)
        {
            BYTE val;
            WORD seg;
            WORD offs;
            int dir;

            if (p[1] >= 0x80)
            {
                dir = -(0x100 - p[1]);
            }
            else
            {
                dir = p[1];
            }
            fprintf(logf, "jl 0%05lXh\n", (unsigned long)((p - base) + 2 + dir));

            if (lt)
            {
                p = p + 2 + dir;
            }
            else
            {
                p += 2; /* this is a 2-byte instruction */
            }
        }
        /* 7305            0171       jnc  fin */
        else if (instr == 0x73)
        {
            BYTE val;
            WORD seg;
            WORD offs;
            FLATAMT fromamt;
            int dir;

            if (p[1] >= 0x80)
            {
                dir = -(0x100 - p[1]);
            }
            else
            {
                dir = p[1];
            }
            fprintf(logf, "jnc 0%05lXh\n", (unsigned long)((p - base) + 2 + dir));

            if (!cflag)
            {
                p = p + 2 + dir;
            }
            else
            {
                p += 2; /* this is a 2-byte instruction */
            }
        }
        /* 3C00            00B3     cmp al,0 */
        else if (instr == 0x3c)
        {
            BYTE val;
            BYTE val2;

            regsize = BYTE_SIZE;
            GET_SRC_REG(REG_AL);
            val = p[1];
            fprintf(logf, "cmp %s,0%02Xh\n", names[REG_AL], (unsigned int)val);
            val2 = FROM_BYTE(src);
            zero = 0;
            /* is this the right way around? */
            if (val == val2)
            {
                lt = 0;
                gt = 0;
                eq = 1;
                zero = 1; /* it seems that equal also sets zero */
            }
            else if (val < val2)
            {
                lt = 1;
                gt = 0;
                eq = 0;
            }
            else
            {
                lt = 0;
                gt = 1;
                eq = 0;
            }
            p += 2;
        }
        /* 3D0001          0049      cmp  ax, 0100h */
        else if (instr == 0x3d)
        {
            WORD val;
            WORD val2;

            regsize = WORD_SIZE;
            GET_SRC_REG(REG_AX);
            val = (p[2] << 8) | p[1];
            fprintf(logf, "cmp %s,0%04Xh\n", names[REG_AX], (unsigned int)val);
            val2 = FROM_BYTE(src);
            zero = 0;
            /* is this the right way around? */
            if (val == val2)
            {
                lt = 0;
                gt = 0;
                eq = 1;
                zero = 1; /* it seems that equal also sets zero */
            }
            else if (val < val2)
            {
                lt = 1;
                gt = 0;
                eq = 0;
            }
            else
            {
                lt = 0;
                gt = 1;
                eq = 0;
            }
            p += 3;
        }
        /* 81C4CCFD add     sp,-564 */
        else if ((instr == 0x81) && (p[1] == 0xc4))
        {
            WORD val;
            WORD val2;

            regsize = WORD_SIZE;
            val = (p[3] << 8) | p[2];
            
            fprintf(logf, "add %s,0%04Xh\n", names[REG_SP], (unsigned int)val);
            
            GET_SRC_REG(REG_SP);
            val2 = FROM_WORD(src);
            
            dest = src;
            
            if (p[3] >= 0x80)
            {
                TO_WORD(dest, val2 - (0x10000UL - val));
            }
            else
            {
                TO_WORD(dest, val2 + val);
            }
            
            p += 4;
        }
        /* 81FB55AA cmp bx,0aa55h */
        else if ((instr == 0x81) && (p[1] == 0xfb))
        {
            WORD val;
            WORD val2;

            if (p[1] != 0xfb)
            {
                printf("unknown register %x\n", p[1]);
                exit(EXIT_FAILURE);
            }
            
            regsize = WORD_SIZE;
            GET_SRC_REG(REG_BX);
            val = (p[3] << 8) | p[2];
            fprintf(logf, "cmp %s,0%04Xh\n", names[REG_BX], (unsigned int)val);
            val2 = FROM_BYTE(src);
            zero = 0;
            /* is this the right way around? */
            if (val == val2)
            {
                lt = 0;
                gt = 0;
                eq = 1;
                zero = 1; /* it seems that equal also sets zero */
            }
            else if (val < val2)
            {
                lt = 1;
                gt = 0;
                eq = 0;
            }
            else
            {
                lt = 0;
                gt = 1;
                eq = 0;
            }
            p += 4;
        }
        /* 817E04A0 00       cmp intnum, 0A0h */
        else if (instr == 0x81)
        {
            WORD val;
            WORD val2;
            WORD seg;
            WORD offs;
            FLATAMT fromamt;
                
            regsize = BYTE_SIZE;
            
            val = (p[4] << 8) | p[3];
            
            GET_SRC_REG(REG_SS);
            seg = FROM_WORD(src);

            offs = (p[2] << 8) | p[1];

            fromamt = GET_FLATAMT(seg, offs);

            fprintf(logf, "cmp byte ptr [%04Xh], 0%04Xh\n", offs, val);
            
            val2 = WORD_AT_MEM(fromamt);
            
            zero = 0;
            /* is this the right way around? */
            if (val == val2)
            {
                lt = 0;
                gt = 0;
                eq = 1;
                zero = 1; /* it seems that equal also sets zero */
            }
            else if (val < val2)
            {
                lt = 1;
                gt = 0;
                eq = 0;
            }
            else
            {
                lt = 0;
                gt = 1;
                eq = 0;
            }
            
            p += 5;
        }
        /* 87DB            00B7     xchg bx,bx */
        else if ((instr == 0x87) && (p[1] == 0xdb))
        {
            fprintf(logf, "xchg bx, bx\n");
            /* effective nop */
            p += 2;
        }
        /* 91              0411           xchg    ax,cx */
        else if (instr == 0x91)
        {
            WORD temp;
            
            fprintf(logf, "xchg ax, cx\n");
            GET_SRC_REG(REG_CX);
            temp = FROM_WORD(src);
            dest = src;
            GET_SRC_REG(REG_AX)
            TO_WORD(dest, FROM_WORD(src));
            dest = src;
            TO_WORD(dest, temp);
            p++;
        }
        /* 90 nop */
        else if (instr == 0x90)
        {
            fprintf(logf, "nop\n");
            p++;
        }
        /* 9c pushf */
        else if (instr == 0x9c)
        {
            WORD val;
            WORD seg;
            WORD offs;
            FLATAMT toamt;
            
            fprintf(logf, "pushf - semi-nop for now\n");
                
            regsize = WORD_SIZE;
            val = 0;

            GET_SRC_REG(REG_SS);
            seg = FROM_WORD(src);
                
            GET_SRC_REG(REG_SP);
            offs = FROM_WORD(src);

            toamt = GET_FLATAMT(seg, offs);
            toamt -= 2;

            dest = src;
            TO_WORD(dest, FROM_WORD(src) - 2);
            WORD_TO_MEM(toamt, val);
            p++;
        }
        /* 9d popf */
        else if (instr == 0x9d)
        {
            WORD val;
            WORD seg;
            WORD offs;
            FLATAMT fromamt;
                
            fprintf(logf, "popf - semi-nop for now\n");
            
            regsize = WORD_SIZE;
            GET_SRC_REG(REG_SS);
            seg = FROM_WORD(src);
                
            GET_SRC_REG(REG_SP);
            offs = FROM_WORD(src);

            fromamt = GET_FLATAMT(seg, offs);

            val = WORD_AT_MEM(fromamt);
            /* need to store this somewhere */
            cflag = 0; /* +++ temporarily clear carry */
            
            dest = src;
            TO_WORD(dest, FROM_WORD(src) + 2);
            p++;

        }
        /* 99              0637           cwd */
        else if (instr == 0x99)
        {
            WORD val;
            
            fprintf(logf, "cwd\n");
            
            GET_SRC_REG(REG_AX);
            val = FROM_WORD(src);
            if (val & 0x8000U)
            {
                val = 0xffffU;
            }
            else
            {
                val = 0;
            }
            GET_DEST_REG(REG_DX);
            TO_WORD(dest, val);
            p++;
        }
        /* F4              00BA     hlt */
        else if (instr == 0xF4)
        {
            fprintf(logf, "hlt\n");
            exit(EXIT_FAILURE);
            p++;
        }
        /* E2F1            0151       loop ReadNextSector */
        else if (instr == 0xe2)
        {
            WORD val;
            int dir;

            if (p[1] >= 0x80)
            {
                dir = -(0x100 - p[1]);
            }
            else
            {
                dir = p[1];
            }
            fprintf(logf, "loop 0%05lXh\n", (unsigned long)((p - base) + 2 + dir));

            GET_SRC_REG(REG_CX);
            dest = src;
            val = FROM_WORD(src);
            val--;
            TO_WORD(dest, val);
            if (val != 0)
            {
                p = p + 2 + dir;
            }
            else
            {
                p += 2; /* this is a 2-byte instruction */
            }
        }
        /* 8D1E1E00 lea bx, newstart */
        /* not sure why this instruction is any different from mov -
           maybe it just doesn't generate relocation info */
        else if ((instr == 0x8d) && (p[1] == 0x1e))
        {
            WORD val;

            regsize = WORD_SIZE;
            GET_DEST_REG(REG_BX);
            val = p[3] << 8 | p[2];
            fprintf(logf, "lea %s,0%04Xh\n", names[REG_BX], (unsigned int)val);
            TO_WORD(dest, val);
            p += 4;
        }
        /* 8D46E0          00D2           lea     ax,[bp-32] */
        else if ((instr == 0x8d) && (p[1] == 0x46))
        {
            WORD val;
            BYTE val2;

            regsize = WORD_SIZE;
            GET_SRC_REG(REG_BP);
            val = FROM_WORD(src);
            val2 = p[2];
            if (p[2] >= 0x80)
            {
                val -= (0x100U - val2);
                fprintf(logf, "lea %s,[bp-0%02Xh]\n",
                        names[REG_AX], (unsigned int)(0x100U - val2));
            }
            else
            {
                val += val2;
                fprintf(logf, "lea %s,[bp+0%02Xh]\n",
                        names[REG_AX], (unsigned int)val2);
            }
            /* even though it looks like I'm supposed to
               indirect, that's not what lea does */
            GET_DEST_REG(REG_AX);
            TO_WORD(dest, val);
            p += 3;
        }
        /* 8D86D2FD lea     ax,[bp-558] */
        else if ((instr == 0x8d) && (p[1] == 0x86))
        {
            WORD val;
            WORD val2;

            regsize = WORD_SIZE;
            GET_SRC_REG(REG_BP);
            val = FROM_WORD(src);
            val2 = p[3] << 8 | p[2];
            if (p[3] >= 0x80)
            {
                val -= (0x10000UL - val2);
                fprintf(logf, "lea %s,[bp-0%04Xh]\n",
                        names[REG_AX], (unsigned int)(0x10000UL - val2));
            }
            else
            {
                val += val2;
                fprintf(logf, "lea %s,[bp+0%04Xh]\n",
                        names[REG_AX], (unsigned int)val2);
            }
            /* do I need to indirect the value against ss or ds or
               does that defeat the purpose of lea vs mov? */
            GET_DEST_REG(REG_AX);
            TO_WORD(dest, val);
            p += 4;
        }
        /* C55E04       lds bx,xaddr */
        /* load ds and bx from the address at bp plus offset */
        else if ((instr == 0xc5) && (p[1] == 0x5e))
        {
            WORD val;
            WORD seg;
            WORD offs;
            FLATAMT fromamt;
                
            regsize = WORD_SIZE;
            
            GET_SRC_REG(REG_BP);
            offs = FROM_WORD(src);
            
            GET_SRC_REG(REG_DS);
            seg = FROM_WORD(src);

            fromamt = GET_FLATAMT(seg, offs);
            
            fromamt += p[2];
            
            val = WORD_AT_MEM(fromamt);

            GET_DEST_REG(REG_BX);
            TO_WORD(dest, val);
            
            fromamt += 2;
            
            val = WORD_AT_MEM(fromamt);

            GET_DEST_REG(REG_DS);
            TO_WORD(dest, val);
            
            fprintf(logf, "lds bx, [bp + 0%02Xh]\n", p[2]);
            
            p += 3;
        }
        /* 29C8            032D           sub     ax,cx */
        else if (instr == 0x29)
        {
            WORD val;
            WORD val2;
            WORD oldval2;
            int r;
            
            regsize = WORD_SIZE;
            if (p[1] == 0xc8)
            {
                r = REG_CX;
            }
            else
            {
                printf("unknown register %x\n", p[1]);
                exit(EXIT_FAILURE);
            }
            fprintf(logf, "sub %s,%s\n", names[REG_AX], names[r]);

            GET_SRC_REG(r);
            val = FROM_WORD(src);
            
            GET_SRC_REG(REG_AX);
            val2 = FROM_WORD(src);
            
            oldval2 = val2;
            val2 -= val;
            if (val2 > oldval2)
            {
                cflag = 1;
            }
            else
            {
                cflag = 0;
            }
            
            dest = src;
            TO_WORD(dest, val2);
            p += 2;
        }
        
        else
        {
            fprintf(logf, "unknown instruction %02X at %08X\n", p[0],
                   p - base);
            exit(EXIT_FAILURE);
        }
    }
    return;
}

        /* 8ED8            mov ds,ax */
        /* 8EC0            mov es,ax */
        /* 8ED0            mov ss,ax */
        /* 8EDA            mov ds,dx */
        /* 8ED2            mov ss,dx */
static void splitregs(unsigned int raw)
{
    unsigned int first;
    unsigned int second;
    
    first = raw >> 3;
    second = raw & 0x7;
    
    if (first == (0xd8 >> 3))
    {
        r1 = REG_DS;
    }
    else if (first == (0xc0 >> 3))
    {
        r1 = REG_ES;
    }
    else if (first == (0xd0 >> 3))
    {
        r1 = REG_SS;
    }
    else
    {
        fprintf(logf, "unknown first register %x\n", raw);
        exit(EXIT_FAILURE);
    }

    if (second == 0x0)
    {
        r2 = REG_AX;
    }
    else if (second == 0x2)
    {
        r2 = REG_DX;
    }
    else
    {
        fprintf(logf, "unknown second register %x\n", raw);
        exit(EXIT_FAILURE);
    }

    if (regsize == WORD_SIZE)
    {
        if (r1 == REG_DS)
        {
            dest = &sregs.ds;
        }
        else if (r1 == REG_ES)
        {
            dest = &sregs.es;
        }
        else if (r1 == REG_SS)
        {
            dest = &sregs.ss;
        }

        if (r2 == REG_DS)
        {
            src = &sregs.ds;
        }
        else if (r2 == REG_ES)
        {
            src = &sregs.es;
        }
        else if (r2 == REG_AX)
        {
            src = &regs.x.ax;
        }
        else if (r2 == REG_DX)
        {
            src = &regs.x.dx;
        }
    }

    return;
}

static void dobios(int intnum)
{
    if (intnum == 0x13)
    {
        cflag = 0;
        /* get drive parms */
        if (regs.h.ah == 0x08)
        {
            printf("GEOMETRY CHECK\n");
            regs.h.cl = 63; /* sectors */
            regs.h.dh = 255-1; /* 255 heads */
            /* ch and cl again should have tracks */
        }
#if 0
        else if (regs.h.ah == 0x00)
        {
            printf("READING failed???\n");
        }
#endif
        /* read */
        else if (regs.h.ah == 0x02)
        {
            unsigned int track;
            unsigned int head;
            unsigned int sector;
            unsigned int num_sectors = 1;
            unsigned long lba;
            
            head = regs.h.dh;
            track = (unsigned int)(regs.h.cl & 0xc0) << 2;
            track += regs.h.ch;
            sector = regs.h.cl & 0x3f;
#if 1
            printf("track %d\n", track);
            printf("head %d\n", head);
            printf("sector %d\n", sector);
#endif
            lba = track * 255 * 63;
            lba += head * 63;
            lba += sector;
            lba--;
            lba *= 512;
            fseek(fp, lba, SEEK_SET);
#if 1
            printf("lba is %lx\n", lba);
            printf("READING to %lx\n", ((FLATAMT)sregs.es << 4) + regs.x.bx);
#endif
            fread(base + ((FLATAMT)sregs.es << 4) + regs.x.bx, 512, 1, fp);
        }
        else if (regs.h.ah == 0x41)
        {
            printf("NO GENUINE LBA CAPABILITY\n");
            regs.x.bx = 0xaa55; /* signify we know the call */
            regs.x.cx = 0; /* but no capabilities for now */
        }
        else
        {
            printf("unknown int 13h %x\n", regs.h.ah);
            exit(EXIT_FAILURE);
        }
    }
    else if (intnum == 0x10)
    {
        if (regs.h.ah == 0x0e)
        {
            printf("CHAR %c\n", regs.h.al);
        }
        else
        {
            printf("unknown int10h\n");
            exit(EXIT_FAILURE);
        }
    }
    return;
}
