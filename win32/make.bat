windres zknock.rc zknockrc.o

gcc -g -c -o zknock.o zknock.c
gcc -g -c -o about.o about.c
gcc -g -c -o addhost.o addhost.c
gcc -g -c -o knockcfg.o knockcfg.c
gcc -g -c -o encrypt.o encrypt.c
gcc -g -c -o knocking.o knocking.c
gcc -g -c -o settings.o settings.c

gcc -mwindows -static -g -o zknock.exe ^
zknock.o about.o addhost.o knockcfg.o ^
encrypt.o knocking.o settings.o zknockrc.o ^
-lcomctl32 -lcrypt32 -lwsock32

rm -f zknock.o zknockrc.o about.o addhost.o knockcfg.o encrypt.o knocking.o
