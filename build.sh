set -xeuo pipefail

arm-none-eabi-gcc -Wall -g -o entry.o -c test.S
arm-none-eabi-gcc -Wall -std=c11 -g -O2 -c ff.c
arm-none-eabi-gcc -Wall -std=c11 -g -O0 -c test.c
arm-none-eabi-gcc -Wall -g -o hax.elf -Wl,-Tlinkscript.ld -nostartfiles -nostdlib test.o entry.o ff.o -lgcc
