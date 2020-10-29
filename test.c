#define OSMR0 (*(volatile unsigned int *)0x40A00000)
#define OSMR1 (*(volatile unsigned int *)0x40A00004)
#define OSMR2 (*(volatile unsigned int *)0x40A00008)
#define OSMR3 (*(volatile unsigned int *)0x40A0000C)
#define OSCR (*(volatile unsigned int *)0x40A00010)
#define OSSR (*(volatile unsigned int *)0x40A00014)
#define OIER (*(volatile unsigned int *)0x40A0001C)

#define GPLR0   (*(volatile unsigned int *)0x40E00000)
#define GPLR1   (*(volatile unsigned int *)0x40E00004)
#define GPLR2   (*(volatile unsigned int *)0x40E00008)
#define GPDR0   (*(volatile unsigned int *)0x40E0000C)
#define GPDR1   (*(volatile unsigned int *)0x40E00010)
#define GPDR2   (*(volatile unsigned int *)0x40E00014)
#define GPSR0   (*(volatile unsigned int *)0x40E00018)
#define GPSR1   (*(volatile unsigned int *)0x40E0001C)
#define GPSR2   (*(volatile unsigned int *)0x40E00020)
#define GPCR0   (*(volatile unsigned int *)0x40E00024)
#define GPCR1   (*(volatile unsigned int *)0x40E00028)
#define GPCR2   (*(volatile unsigned int *)0x40E0002C)
#define GAFR0_L (*(volatile unsigned int *)0x40E00054)
#define GAFR0_U (*(volatile unsigned int *)0x40E00058)
#define GAFR1_L (*(volatile unsigned int *)0x40E0005C)
#define GAFR1_U (*(volatile unsigned int *)0x40E00060)
#define GAFR2_L (*(volatile unsigned int *)0x40E00064)
#define GAFR2_U (*(volatile unsigned int *)0x40E00068)

#define MMC_STRPCL  (*(volatile unsigned int *)0x41100000)
#define MMC_STAT    (*(volatile unsigned int *)0x41100004)
#define MMC_CLKRT   (*(volatile unsigned int *)0x41100008)
#define MMC_SPI     (*(volatile unsigned int *)0x4110000C)
#define MMC_CMDAT   (*(volatile unsigned int *)0x41100010)
#define MMC_RESTO   (*(volatile unsigned int *)0x41100014)
#define MMC_RDTO    (*(volatile unsigned int *)0x41100018)
#define MMC_BLKLEN  (*(volatile unsigned int *)0x4110001C)
#define MMC_NOB     (*(volatile unsigned int *)0x41100020)
#define MMC_PRTBUF  (*(volatile unsigned int *)0x41100024)
#define MMC_I_MASK  (*(volatile unsigned int *)0x41100028)
#define MMC_I_REG   (*(volatile unsigned int *)0x4110002C)
#define MMC_CMD     (*(volatile unsigned int *)0x41100030)
#define MMC_ARGH    (*(volatile unsigned int *)0x41100034)
#define MMC_ARGL    (*(volatile unsigned int *)0x41100038)
#define MMC_RES     (*(volatile unsigned int *)0x4110003C)
#define MMC_RXFIFO  (*(volatile unsigned int *)0x41100040)
#define MMC_TXFIFO  (*(volatile unsigned int *)0x41100044)

#define CKEN (*(volatile unsigned int *)0x41300004)

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

    // Setup SD card
    CKEN = (1 << 12);
    GPDR0 = (1 << 6) | (1 << 8) | (1 << 9);
    GAFR0_L = (1 << 12) | (1 << 16) | (1 << 18);
    MMC_I_MASK = 0;
    MMC_CLKRT = 0b110;

    debug_str("setup ");

    // not in specs or anything, but 10 ms init clocks
    MMC_STRPCL = 2;
    msleep(10);
    MMC_STRPCL = 1;
    while ((MMC_I_REG & (1 << 4)) == 0) {}

    debug_str("clocks ");

    // GO_IDLE_STATE
    MMC_CMD = 0;
    MMC_ARGH = 0;
    MMC_ARGL = 0;
    MMC_CMDAT = 1 << 6; // INIT
    MMC_STRPCL = 2;
    while ((MMC_I_REG & (1 << 2)) == 0) {}
    MMC_STRPCL = 1;
    while ((MMC_I_REG & (1 << 4)) == 0) {}

    debug_str("goidle ");

    // SEND_IF_COND
    MMC_CMD = 8;
    MMC_ARGH = 0;
    MMC_ARGL = 0x1AA;
    MMC_CMDAT = 1;
    MMC_STRPCL = 2;
    while ((MMC_I_REG & (1 << 2)) == 0) {}

    debug_str("ifcond ");

    if (MMC_STAT & (1 << 1)) {
        debug_str("timeout ");
    } else if (MMC_STAT & (1 << 5)) {
        debug_str("crc error ");
    } else {
        debug_32(MMC_RES);
        debug_32(MMC_RES);
        debug_32(MMC_RES);
    }

    MMC_STRPCL = 1;
    while ((MMC_I_REG & (1 << 4)) == 0) {}

    unsigned int tries = 100;
    while(tries--) {
        MMC_CMD = 55;
        MMC_ARGH = 0;
        MMC_ARGL = 0;
        MMC_CMDAT = 1;
        MMC_STRPCL = 2;
        while ((MMC_I_REG & (1 << 2)) == 0) {}

        debug_str("app_cmd ");

        if (MMC_STAT & (1 << 1)) {
            debug_str("timeout ");
            MMC_STRPCL = 1;
            while ((MMC_I_REG & (1 << 4)) == 0) {}
            continue;
        } else if (MMC_STAT & (1 << 5)) {
            debug_str("crc error ");
            MMC_STRPCL = 1;
            while ((MMC_I_REG & (1 << 4)) == 0) {}
            continue;
        } else {
            debug_32(MMC_RES);
            debug_32(MMC_RES);
            debug_32(MMC_RES);
        }

        MMC_STRPCL = 1;
        while ((MMC_I_REG & (1 << 4)) == 0) {}


        MMC_CMD = 41;
        MMC_ARGH = 0x4030;
        MMC_ARGL = 0;
        MMC_CMDAT = 3;
        MMC_STRPCL = 2;
        while ((MMC_I_REG & (1 << 2)) == 0) {}

        debug_str("opcond ");

        if (MMC_STAT & (1 << 1)) {
            debug_str("timeout ");
            msleep(10);
            MMC_STRPCL = 1;
            while ((MMC_I_REG & (1 << 4)) == 0) {}
            continue;
        } else {
            debug_32(MMC_RES);
            debug_32(MMC_RES);
            debug_32(MMC_RES);
        }

        MMC_STRPCL = 1;
        while ((MMC_I_REG & (1 << 4)) == 0) {}
    }

    while (1) {}
}
