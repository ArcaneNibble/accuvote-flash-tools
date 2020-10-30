set -xeuo pipefail

arm-none-eabi-gcc -Wall -g -o entry.o -c test.S
arm-none-eabi-gcc -Wall -std=c11 -g -O2 -ffunction-sections -fdata-sections -c ff.c
arm-none-eabi-gcc -Wall -std=c11 -g -O0 -ffunction-sections -fdata-sections -c test.c
arm-none-eabi-gcc -Wall -g -o hax.elf -Wl,-Tlinkscript.ld -Wl,--gc-sections -nostartfiles -nostdlib test.o entry.o ff.o -lgcc
