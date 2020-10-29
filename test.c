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

void entry() {
    debug_str("Hello World! ");
    debug_32(0xcafef00d);
    debug_str(" ");
    debug_8(0xa5);
    debug_str(" OwO");

    while (1) {}
}
