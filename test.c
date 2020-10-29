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
        if (debugbuf_idx == sizeof(debugbuf))
            debugbuf_idx = 0;
        debugbuf[debugbuf_idx++] = c;
    }
}

const char *hexlut = "0123456789ABCDEF";

void debug_32(unsigned int x) {
    for (int i = 0; i < 8; i++) {
        if (debugbuf_idx == sizeof(debugbuf))
            debugbuf_idx = 0;
        debugbuf[debugbuf_idx++] = hexlut[(x >> ((7 - i) * 4)) & 0xF];
    }
}

void debug_16(unsigned short x) {
    for (int i = 0; i < 4; i++) {
        if (debugbuf_idx == sizeof(debugbuf))
            debugbuf_idx = 0;
        debugbuf[debugbuf_idx++] = hexlut[(x >> ((3 - i) * 4)) & 0xF];
    }
}

void debug_8(unsigned char x) {
    if (debugbuf_idx == sizeof(debugbuf))
        debugbuf_idx = 0;
    debugbuf[debugbuf_idx++] = hexlut[(x >> 4) & 0xF];
    if (debugbuf_idx == sizeof(debugbuf))
        debugbuf_idx = 0;
    debugbuf[debugbuf_idx++] = hexlut[(x >> 0) & 0xF];
}

void msleep(unsigned int ms) {
    OSSR = 1;   // clear match
    OSMR0 = OSCR + 3687 * ms;
    while ((OSSR & 1) == 0) {}
}

void mmc_start_clk() {
    MMC_STRPCL = 2;
}

void mmc_stop_clk() {
    MMC_STRPCL = 1;
    while ((MMC_I_REG & (1 << 4)) == 0) {}
}

void mmc_do_cmd(unsigned char cmd, unsigned int arg,
                int init, int busy, int format) {
    MMC_CMD = cmd;
    MMC_ARGH = arg >> 16;
    MMC_ARGL = arg & 0xFFFF;

    unsigned int cmdat = format;
    if (init)
        cmdat |= 1 << 6;
    if (busy)
        cmdat |= 1 << 5;

    MMC_CMDAT = cmdat;

    mmc_start_clk();
    while ((MMC_I_REG & (1 << 2)) == 0) {}
}

int mmc_finish_r1(unsigned char cmd, unsigned int *out) {
    if (MMC_STAT & (1 << 1)) {
        mmc_stop_clk();
        return 0;
    } else if (MMC_STAT & (1 << 5)) {
        mmc_stop_clk();
        return 0;
    } else {
        unsigned int ret;
        unsigned int tmp = MMC_RES & 0xFFFF;
        if (tmp >> 8 != cmd)
            return 0;
        ret = (tmp & 0xFF) << 24;

        tmp = MMC_RES & 0xFFFF;
        ret |= tmp << 8;

        tmp = MMC_RES & 0xFFFF;
        ret |= tmp >> 8;

        *out = ret;

        mmc_stop_clk();
        return 1;
    }
}

int mmc_finish_r2(unsigned int *out) {
    if (MMC_STAT & (1 << 1)) {
        mmc_stop_clk();
        return 0;
    } else if (MMC_STAT & (1 << 5)) {
        mmc_stop_clk();
        return 0;
    } else {
        unsigned int ret;
        unsigned int tmp = MMC_RES & 0xFFFF;
        if (tmp >> 8 != 0x3F)
            return 0;
        ret = (tmp & 0xFF) << 24;

        tmp = MMC_RES & 0xFFFF;
        ret |= tmp << 8;

        tmp = MMC_RES & 0xFFFF;
        ret |= tmp >> 8;
        
        out[0] = ret;

        ret = (tmp & 0xFF) << 24;

        tmp = MMC_RES & 0xFFFF;
        ret |= tmp << 8;

        tmp = MMC_RES & 0xFFFF;
        ret |= tmp >> 8;

        out[1] = ret;

        ret = (tmp & 0xFF) << 24;

        tmp = MMC_RES & 0xFFFF;
        ret |= tmp << 8;

        tmp = MMC_RES & 0xFFFF;
        ret |= tmp >> 8;

        out[2] = ret;

        ret = (tmp & 0xFF) << 24;

        tmp = MMC_RES & 0xFFFF;
        ret |= tmp << 8;

        // CRC cannot be read
        
        out[3] = ret;

        mmc_stop_clk();
        return 1;
    }
}

int mmc_finish_r3(unsigned int *out) {
    if (MMC_STAT & (1 << 1)) {
        mmc_stop_clk();
        return 0;
    }
    else {
        unsigned int ret;
        unsigned int tmp = MMC_RES & 0xFFFF;
        if (tmp >> 8 != 0x3F)
            return 0;
        ret = (tmp & 0xFF) << 24;

        tmp = MMC_RES & 0xFFFF;
        ret |= tmp << 8;

        tmp = MMC_RES & 0xFFFF;
        ret |= tmp >> 8;
        
        *out = ret;

        mmc_stop_clk();
        return 1;
    }
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
    mmc_start_clk();
    msleep(10);
    mmc_stop_clk();
    debug_str("clocks ");

    // GO_IDLE_STATE
    mmc_do_cmd(0, 0, 1, 0, 0);
    mmc_stop_clk();
    debug_str("goidle ");

    // SEND_IF_COND
    mmc_do_cmd(8, 0x1AA, 0, 0, 1);
    unsigned int ifcond;
    int ret = mmc_finish_r1(8, &ifcond);
    debug_str("ifcond ");
    int should_try_sdhc = 1;
    if (!ret) {
        debug_str("err ");
        should_try_sdhc = 0;
    } else {
        debug_32(ifcond);
        debug_str(" ");
    }

    unsigned int tries = 100;
    int init_ok = 0;
    int is_sdhc = 0;
    while(tries--) {
        mmc_do_cmd(55, 0, 0, 0, 1);
        unsigned int app_cmd;
        ret = mmc_finish_r1(55, &app_cmd);
        debug_str("app_cmd ");
        if (!ret) {
            debug_str("err ");
            continue;
        } else {
            debug_32(app_cmd);
            debug_str(" ");
        }


        // SEND_OP_COND
        mmc_do_cmd(41, should_try_sdhc ? 0x40300000 : 0x00300000, 0, 0, 3);
        msleep(10);
        unsigned int opcond;
        ret = mmc_finish_r3(&opcond);
        debug_str("opcond ");

        if (!ret) {
            debug_str("err ");
            continue;
        } else {
            debug_32(opcond);
            debug_str(" ");
        }

        if (opcond & 0x80000000) {
            debug_str("acmd41_ok! ");
            init_ok = 1;
            is_sdhc = opcond & 0x40000000;
            break;
        }
    }

    if (!init_ok) {
        while (1) {}
    }

    // ALL_SEND_CID
    mmc_do_cmd(2, 0, 0, 0, 2);
    unsigned int cid[4];
    ret = mmc_finish_r2(cid);
    debug_str("cid ");
    if (!ret) {
        debug_str("err ");
    } else {
        debug_32(cid[0]);
        debug_32(cid[1]);
        debug_32(cid[2]);
        debug_32(cid[3]);
        debug_str(" ");
    }

    // SEND_RELATIVE_ADDR
    mmc_do_cmd(3, 0, 0, 0, 1);
    unsigned int rca;
    ret = mmc_finish_r1(3, &rca);
    debug_str("rca ");
    if (!ret) {
        debug_str("err ");
        while (1) {}
    } else {
        debug_32(rca);
        debug_str(" ");
    }
    rca &= 0xFFFF0000;

    // SEND_CSD
    mmc_do_cmd(9, rca, 0, 0, 2);
    unsigned int csd[4];
    ret = mmc_finish_r2(csd);
    debug_str("csd ");
    if (!ret) {
        debug_str("err ");
    } else {
        debug_32(csd[0]);
        debug_32(csd[1]);
        debug_32(csd[2]);
        debug_32(csd[3]);
        debug_str(" ");
    }

    // SELECT_CARD
    mmc_do_cmd(7, rca, 0, 1, 1);
    unsigned int status;
    ret = mmc_finish_r1(7, &status);
    debug_str("select ");
    if (!ret) {
        debug_str("err ");
        while (1) {}
    } else {
        debug_32(status);
        debug_str(" ");
    }

    // SET_BLOCKLEN
    mmc_do_cmd(16, 512, 0, 0, 1);
    ret = mmc_finish_r1(16, &status);
    debug_str("blocklen ");
    if (!ret) {
        debug_str("err ");
    } else {
        debug_32(status);
        debug_str(" ");
    }

    while ((status & 0x1F00) != 0x900) {
        // SEND_STATUS
        mmc_do_cmd(13, rca, 0, 0, 1);
        ret = mmc_finish_r1(13, &status);
        debug_str("status ");
        if (!ret) {
            debug_str("err ");
        } else {
            debug_32(status);
            debug_str(" ");
        }
    }

    debug_str("sd_init_all_ok! ");

    while (1) {}
}
