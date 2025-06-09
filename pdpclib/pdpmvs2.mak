all:
  pdcc -N -D MVS -o MFSUPA.S mfsupa.asm
  as370 -mhlasm -mebcdic -o MFSUPA.OBJ mfsupa.s
  gccmvs -Os -I. -I../generic -I../src -S -o MFSUPC.S mfsupc.c
  as370 -mhlasm -mebcdic -o MFSUPC.OBJ mfsupc.s
  pdld -e __crt0 --oformat mvs -o PDPTEST.EXE mfsupa.obj mfsupc.obj
