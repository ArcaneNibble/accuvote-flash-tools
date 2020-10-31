set -xeuo pipefail

arm-none-eabi-gcc -Wall -g -o entry.o -c test.S
arm-none-eabi-gcc -Wall -std=c11 -g -O2 -ffunction-sections -fdata-sections -c ff.c

arm-none-eabi-gcc -DDUMPFLASH -Wall -std=c11 -g -O2 -ffunction-sections -fdata-sections -o dump.o -c test.c
arm-none-eabi-gcc -Wall -g -O2 -o dump.elf -Wl,-Tlinkscript.ld -Wl,--gc-sections -nostartfiles -nostdlib dump.o entry.o ff.o -lgcc

arm-none-eabi-gcc -DLOADFLASH -fno-delete-null-pointer-checks -Wall -std=c11 -g -O2 -ffunction-sections -fdata-sections -o load.o -c test.c
arm-none-eabi-gcc -Wall -g -O2 -o load.elf -Wl,-Tlinkscript.ld -Wl,--gc-sections -nostartfiles -nostdlib load.o entry.o ff.o -lgcc
