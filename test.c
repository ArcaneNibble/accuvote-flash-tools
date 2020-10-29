#define OSMR0 (*(volatile unsigned int *)0x40A00000)
#define OSMR1 (*(volatile unsigned int *)0x40A00004)
#define OSMR2 (*(volatile unsigned int *)0x40A00008)
#define OSMR3 (*(volatile unsigned int *)0x40A0000C)
#define OSCR (*(volatile unsigned int *)0x40A00010)
#define OSSR (*(volatile unsigned int *)0x40A00014)
#define OIER (*(volatile unsigned int *)0x40A0001C)

int debugbuf_idx;
char debugbuf[4096];

void debug_str(const char *s) {
    char c;
    while ((c = *(s++))) {
        debugbuf[debugbuf_idx++] = c;
    }
}

const char *hexlut = "0123456789ABCDEF";

void debug_32(unsigned int x) {
    for (int i = 0; i < 8; i++) {
        debugbuf[debugbuf_idx++] = hexlut[(x >> ((7 - i) * 4)) & 0xF];
    }
}

void debug_8(unsigned char x) {
    debugbuf[debugbuf_idx++] = hexlut[(x >> 4) & 0xF];
    debugbuf[debugbuf_idx++] = hexlut[(x >> 0) & 0xF];
}

void msleep(unsigned int ms) {
    OSSR = 1;   // clear match
    OSMR0 = OSCR + 3687 * ms;
    while ((OSSR & 1) == 0) {}
}

void entry() {
    // Setup timer
    OIER = 1;

    debug_str("Hello World! ");
    debug_32(0xcafef00d);

    msleep(30000);

    debug_str(" ");
    debug_8(0xa5);
    debug_str(" OwO");

    while (1) {}
}
