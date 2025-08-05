//MVSPB    JOB CLASS=C,REGION=0K,TIME=1440
//*
//*
//*
//MINI     EXEC PGM=MINIUNZ,PARM='dd:in dd:out'
//STEPLIB  DD DSN=MINIZIP.LINKLIB,DISP=SHR
//IN       DD  DSN=HERC01.IN,UNIT=TAPE,VOL=SER=PCTOMF,LABEL=(1,NL),
//  DCB=(RECFM=U,LRECL=0,BLKSIZE=32760)
//OUT      DD  DSN=&&UNZIP,DISP=(,PASS),
// DCB=(RECFM=U,LRECL=0,BLKSIZE=6233),
// SPACE=(CYL,(10,10,20)),UNIT=SYSALLDA
//SYSIN    DD DUMMY
//SYSPRINT DD SYSOUT=*
//SYSTERM  DD SYSOUT=*
//*
//*
//*
//COPYFILE EXEC PGM=COPYFILE,PARM='-bb dd:in dd:out'
//SYSPRINT DD  SYSOUT=*
//SYSTERM  DD  SYSOUT=*
//SYSABEND DD  SYSOUT=*
//SYSIN    DD  DUMMY
//IN       DD  DSN=&&UNZIP(BIOS),DISP=(OLD,PASS)
//OUT      DD  DSN=&&UNLOAD,DISP=(,PASS),
// DCB=(RECFM=VS,LRECL=32756,BLKSIZE=6233),
// SPACE=(CYL,(10,10)),UNIT=SYSALLDA
//*
//*
//*
//IEBCOPY  EXEC PGM=IEBCOPY
//I1       DD  DSN=&&UNLOAD,DISP=(OLD,PASS)
//I2       DD  DSN=&&LOADLIB,DISP=(,PASS),
// DCB=(RECFM=U,LRECL=0,BLKSIZE=6144),
// SPACE=(CYL,(10,10,20)),UNIT=SYSALLDA
//SYSPRINT DD  SYSOUT=*
//SYSIN    DD  *
 COPY INDD=((I1,R)),OUTDD=I2
/*
//*
//*
//*
//CPYPCOMM EXEC PGM=COPYFILE,PARM='-bb dd:in dd:out'
//SYSPRINT DD  SYSOUT=*
//SYSTERM  DD  SYSOUT=*
//SYSABEND DD  SYSOUT=*
//SYSIN    DD  DUMMY
//IN       DD  DSN=&&UNZIP(PCOMM),DISP=(OLD,PASS)
//OUT      DD  DSN=&&PCOMM,DISP=(,PASS),
// DCB=(RECFM=U,LRECL=0,BLKSIZE=6233),
// SPACE=(CYL,(10,10)),UNIT=SYSALLDA
//*
//*
//*
//BIOS     EXEC PGM=BIOS,PARM=''
//STEPLIB  DD  DSN=&&LOADLIB,DISP=(OLD,PASS)
//SYSPRINT DD  SYSOUT=*
//SYSTERM  DD  SYSOUT=*
//SYSABEND DD  SYSOUT=*
//PDOS     DD  DSN=&&UNZIP(PDOS),DISP=(SHR,PASS)
//*
//* The dd:pdos needs to be a host filename
//* The dd:config is currently kludged and needs to be
//* a host file with : in it, which happens to work
//* The dd:dasd needs to be a host filename
//* The dd:pcomm is a guest filename, but we don't want
//* that at the moment, so we should ideally prefix
//* with a : to signify to go up one level
//* current logic sends it up anyway, but we shouldn't
//* be relying on that
//*
//SYSIN    DD  *
dd:pdos -c dd:config dd:dasd
help
date
type :dd:xyz
type :dd:xyz
type readme.txt
type :dd:xyz
dir
type dd:xyz
copy dd:xyz TESTCOPY.TXT
dir
type testcopy.txt
gccmvs --version
copy test.c FRED.C
type fred.c
dir
gccmvs -S -Os -o FRED.S fred.c
dir
type fred.s
exit
/*
//CONFIG   DD  *
COMSPEC=:dd:pcomm
/*
//XYZ      DD  *
fred was here
/*
//PCOMM    DD  DSN=&&PCOMM,DISP=(OLD,PASS)
//* We get "dataset not found" if we try the below
//* Apparently an MVS restriction on using temporary
//* datasets. Not sure if it can be gotten around with
//* a reference or something. In the meantime, we just
//* copy it into a new temporary dataset first
//*PCOMM     DD  DSN=&&UNZIP(PCOMM),DISP=(SHR,PASS)
//DASD     DD  DSN=FBA1B1.VHD,DISP=OLD,
//    UNIT=1B6,VOL=SER=D33902
//*
//
