#include <stdint.h>

#include "ff.h"
#include "diskio.h"

#define FFRBR (*(volatile uint32_t *)0x40100000)
#define FFTHR (*(volatile uint32_t *)0x40100000)
#define FFIER (*(volatile uint32_t *)0x40100004)
#define FFIIR (*(volatile uint32_t *)0x40100008)
#define FFFCR (*(volatile uint32_t *)0x40100008)
#define FFLCR (*(volatile uint32_t *)0x4010000C)
#define FFMCR (*(volatile uint32_t *)0x40100010)
#define FFLSR (*(volatile uint32_t *)0x40100014)
#define FFMSR (*(volatile uint32_t *)0x40100018)
#define FFSPR (*(volatile uint32_t *)0x4010001C)
#define FFISR (*(volatile uint32_t *)0x40100020)
#define FFDLL (*(volatile uint32_t *)0x40100000)
#define FFDLH (*(volatile uint32_t *)0x40100004)

#define OSMR0 (*(volatile uint32_t *)0x40A00000)
#define OSMR1 (*(volatile uint32_t *)0x40A00004)
#define OSMR2 (*(volatile uint32_t *)0x40A00008)
#define OSMR3 (*(volatile uint32_t *)0x40A0000C)
#define OSCR (*(volatile uint32_t *)0x40A00010)
#define OSSR (*(volatile uint32_t *)0x40A00014)
#define OIER (*(volatile uint32_t *)0x40A0001C)

#define GPLR0   (*(volatile uint32_t *)0x40E00000)
#define GPLR1   (*(volatile uint32_t *)0x40E00004)
#define GPLR2   (*(volatile uint32_t *)0x40E00008)
#define GPDR0   (*(volatile uint32_t *)0x40E0000C)
#define GPDR1   (*(volatile uint32_t *)0x40E00010)
#define GPDR2   (*(volatile uint32_t *)0x40E00014)
#define GPSR0   (*(volatile uint32_t *)0x40E00018)
#define GPSR1   (*(volatile uint32_t *)0x40E0001C)
#define GPSR2   (*(volatile uint32_t *)0x40E00020)
#define GPCR0   (*(volatile uint32_t *)0x40E00024)
#define GPCR1   (*(volatile uint32_t *)0x40E00028)
#define GPCR2   (*(volatile uint32_t *)0x40E0002C)
#define GAFR0_L (*(volatile uint32_t *)0x40E00054)
#define GAFR0_U (*(volatile uint32_t *)0x40E00058)
#define GAFR1_L (*(volatile uint32_t *)0x40E0005C)
#define GAFR1_U (*(volatile uint32_t *)0x40E00060)
#define GAFR2_L (*(volatile uint32_t *)0x40E00064)
#define GAFR2_U (*(volatile uint32_t *)0x40E00068)

#define MMC_STRPCL  (*(volatile uint32_t *)0x41100000)
#define MMC_STAT    (*(volatile uint32_t *)0x41100004)
#define MMC_CLKRT   (*(volatile uint32_t *)0x41100008)
#define MMC_SPI     (*(volatile uint32_t *)0x4110000C)
#define MMC_CMDAT   (*(volatile uint32_t *)0x41100010)
#define MMC_RESTO   (*(volatile uint32_t *)0x41100014)
#define MMC_RDTO    (*(volatile uint32_t *)0x41100018)
#define MMC_BLKLEN  (*(volatile uint32_t *)0x4110001C)
#define MMC_NOB     (*(volatile uint32_t *)0x41100020)
#define MMC_PRTBUF  (*(volatile uint32_t *)0x41100024)
#define MMC_I_MASK  (*(volatile uint32_t *)0x41100028)
#define MMC_I_REG   (*(volatile uint32_t *)0x4110002C)
#define MMC_CMD     (*(volatile uint32_t *)0x41100030)
#define MMC_ARGH    (*(volatile uint32_t *)0x41100034)
#define MMC_ARGL    (*(volatile uint32_t *)0x41100038)
#define MMC_RES     (*(volatile uint32_t *)0x4110003C)
#define MMC_RXFIFO  (*(volatile uint32_t *)0x41100040)
#define MMC_TXFIFO  (*(volatile uint32_t *)0x41100044)

#define CKEN (*(volatile uint32_t *)0x41300004)

void uart_putc(char c) {
    while ((FFLSR & (1 << 5)) == 0) {}
    FFTHR = ((unsigned int)c) & 0xFF;
}

void debug_str(const char *s) {
    char c;
    while ((c = *(s++))) {
        uart_putc(c);
    }
}

const char *hexlut = "0123456789ABCDEF";

void debug_32(uint32_t x) {
    for (int i = 0; i < 8; i++) {
        uart_putc(hexlut[(x >> ((7 - i) * 4)) & 0xF]);
    }
}

void debug_16(uint16_t x) {
    for (int i = 0; i < 4; i++) {
        uart_putc(hexlut[(x >> ((3 - i) * 4)) & 0xF]);
    }
}

void debug_8(uint8_t x) {
    uart_putc(hexlut[(x >> 4) & 0xF]);
    uart_putc(hexlut[(x >> 0) & 0xF]);
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

void mmc_do_cmd(uint8_t cmd, uint32_t arg,
                int init, int busy, uint32_t format) {
    MMC_CMD = cmd;
    MMC_ARGH = arg >> 16;
    MMC_ARGL = arg & 0xFFFF;

    uint32_t cmdat = format;
    if (init)
        cmdat |= 1 << 6;
    if (busy)
        cmdat |= 1 << 5;

    MMC_CMDAT = cmdat;

    mmc_start_clk();
    while ((MMC_I_REG & (1 << 2)) == 0) {}
}

int mmc_finish_r1(uint8_t cmd, uint32_t *out) {
    if (MMC_STAT & (1 << 1)) {
        mmc_stop_clk();
        return 0;
    } else if (MMC_STAT & (1 << 5)) {
        mmc_stop_clk();
        return 0;
    } else {
        uint32_t ret;
        uint32_t tmp = MMC_RES & 0xFFFF;
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

int mmc_finish_r2(uint32_t *out) {
    if (MMC_STAT & (1 << 1)) {
        mmc_stop_clk();
        return 0;
    } else if (MMC_STAT & (1 << 5)) {
        mmc_stop_clk();
        return 0;
    } else {
        uint32_t ret;
        uint32_t tmp = MMC_RES & 0xFFFF;
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

int mmc_finish_r3(uint32_t *out) {
    if (MMC_STAT & (1 << 1)) {
        mmc_stop_clk();
        return 0;
    }
    else {
        uint32_t ret;
        uint32_t tmp = MMC_RES & 0xFFFF;
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

int is_sdhc;

int mmc_read_block(unsigned int block,
                   uint8_t *outbuf, uint32_t *outstatus) {
    MMC_CMD = 17;
    if (!is_sdhc)
        block *= 512;
    MMC_ARGH = (block >> 16) & 0xFFFF;
    MMC_ARGL = block & 0xFFFF;
    MMC_CMDAT = 1 << 2 | 1;
    mmc_start_clk();
    while ((MMC_I_REG & (1 << 2)) == 0) {}

    if (MMC_STAT & (1 << 1)) {
        mmc_stop_clk();
        return 0;
    } else if (MMC_STAT & (1 << 5)) {
        mmc_stop_clk();
        return 0;
    } else {
        // response
        uint32_t ret;
        uint32_t tmp = MMC_RES & 0xFFFF;
        if (tmp >> 8 != 17) {
            mmc_stop_clk();
            return 0;
        }
        ret = (tmp & 0xFF) << 24;

        tmp = MMC_RES & 0xFFFF;
        ret |= tmp << 8;

        tmp = MMC_RES & 0xFFFF;
        ret |= tmp >> 8;

        *outstatus = ret;

        // data
        for (int i = 0; i < 512; i++) {
            while((MMC_I_REG & (1 << 5)) == 0) {}
            outbuf[i] = MMC_RXFIFO;
        }
        while ((MMC_I_REG & 1) == 0) {}

        tmp = MMC_STAT;
        mmc_stop_clk();

        if (tmp & (1 << 3))
            return 0;

        return 1;
    }
}

int mmc_write_block(unsigned int block,
                    const uint8_t *outbuf, uint32_t *outstatus) {
    MMC_CMD = 24;
    if (!is_sdhc)
        block *= 512;
    MMC_ARGH = (block >> 16) & 0xFFFF;
    MMC_ARGL = block & 0xFFFF;
    MMC_CMDAT = (1 << 3) | (1 << 2) | 1;
    mmc_start_clk();
    while ((MMC_I_REG & (1 << 2)) == 0) {}

    if (MMC_STAT & (1 << 1)) {
        mmc_stop_clk();
        return 0;
    } else if (MMC_STAT & (1 << 5)) {
        mmc_stop_clk();
        return 0;
    } else {
        // response
        uint32_t ret;
        uint32_t tmp = MMC_RES & 0xFFFF;
        if (tmp >> 8 != 24) {
            mmc_stop_clk();
            return 0;
        }
        ret = (tmp & 0xFF) << 24;

        tmp = MMC_RES & 0xFFFF;
        ret |= tmp << 8;

        tmp = MMC_RES & 0xFFFF;
        ret |= tmp >> 8;

        *outstatus = ret;

        // data
        for (int i = 0; i < 512; i++) {
            while((MMC_I_REG & (1 << 6)) == 0) {}
            MMC_TXFIFO = outbuf[i];
        }
        while ((MMC_I_REG & 1) == 0) {}

        // prg_done
        while ((MMC_I_REG & (1 << 1)) == 0) {}

        tmp = MMC_STAT;
        mmc_stop_clk();

        if (tmp & (1 << 2))
            return 0;

        return 1;
    }
}

void set_green(int state) {
    if (state) {
        GPCR0 = 1 << 2;
    } else {
        GPSR0 = 1 << 2;
    }
}

void set_red(int state) {
    if (state) {
        GPCR0 = 1 << 3;
    } else {
        GPSR0 = 1 << 3;
    }
}

void set_backlight(int state) {
    if (state) {
        GPSR1 = 1 << 30;
    } else {
        GPCR1 = 1 << 30;
    }
}

FATFS fs;
FIL fil;

DRESULT disk_read (
    BYTE pdrv,      /* Physical drive number to identify the drive */
    BYTE *buff,     /* Data buffer to store read data */
    LBA_t sector,   /* Start sector in LBA */
    UINT count      /* Number of sectors to read */
) {
    for (int i = 0; i < count; i++) {
        uint32_t status;
        int ret = mmc_read_block(sector + i, buff + (i * 512), &status);

        if (!ret)
            return RES_ERROR;
    }

    return RES_OK;
}

DRESULT disk_write (
    BYTE pdrv,          /* Physical drive nmuber to identify the drive */
    const BYTE *buff,   /* Data to be written */
    LBA_t sector,       /* Start sector in LBA */
    UINT count          /* Number of sectors to write */
) {
    for (int i = 0; i < count; i++) {
        uint32_t status;
        int ret = mmc_write_block(sector + i, buff + (i * 512), &status);

        if (!ret)
            return RES_ERROR;
    }

    return RES_OK;
}

DSTATUS disk_status (
    BYTE pdrv       /* Physical drive nmuber to identify the drive */
) {
    return 0;
}

DSTATUS disk_initialize (
    BYTE pdrv               /* Physical drive nmuber to identify the drive */
) {
    return 0;
}

DRESULT disk_ioctl (
    BYTE pdrv,      /* Physical drive nmuber (0..) */
    BYTE cmd,       /* Control code */
    void *buff      /* Buffer to send/receive control data */
) {
    if (cmd == CTRL_SYNC)
        return RES_OK;

    return RES_PARERR;
}

void panic() {
    while (1) {
        set_red(1);
        msleep(125);
        set_red(0);
        msleep(125);
    }
}

void entry() {
    // Setup timer
    OIER = 1;

    // Setup GPIOs
    set_green(0);
    set_red(0);
    set_backlight(0);
    GPDR0 =
        (1 << 6) | (1 << 8) | (1 << 9) |    // MMC
        (1 << 2) | (1 << 3);                // LEDs
    GPDR1 =
        (1 << 30) |                         // Backlight
        (1 << 7);                           // Modem UART
    GAFR0_L = (1 << 12) | (1 << 16) | (1 << 18);    // MMC
    GAFR1_L = (1 << 4) | (2 << 14);                 // UART

    CKEN = (1 << 6) | (1 << 12);

    // Setup SD card
    MMC_I_MASK = 0;
    MMC_CLKRT = 0b110;
    MMC_NOB = 1;
    MMC_BLKLEN = 512;

    // Setup UART
    FFLCR = 3 | (1 << 7);
    FFDLL = 8;
    FFDLH = 0;
    FFLCR = 3;
    FFIER = 1 << 6;

    debug_str("\r\n\r\nHello world!\r\n");

    debug_str("setup\r\n");

    // not in specs or anything, but 10 ms init clocks
    mmc_start_clk();
    msleep(10);
    mmc_stop_clk();
    debug_str("clocks\r\n");

    // GO_IDLE_STATE
    mmc_do_cmd(0, 0, 1, 0, 0);
    mmc_stop_clk();
    debug_str("goidle\r\n");

    // SEND_IF_COND
    mmc_do_cmd(8, 0x1AA, 0, 0, 1);
    uint32_t ifcond;
    int ret = mmc_finish_r1(8, &ifcond);
    debug_str("ifcond ");
    int should_try_sdhc = 1;
    if (!ret) {
        debug_str("err\r\n");
        should_try_sdhc = 0;
    } else {
        debug_32(ifcond);
        debug_str("\r\n");
    }

    unsigned int tries = 100;
    int init_ok = 0;
    is_sdhc = 0;
    while(tries--) {
        mmc_do_cmd(55, 0, 0, 0, 1);
        uint32_t app_cmd;
        ret = mmc_finish_r1(55, &app_cmd);
        debug_str("app_cmd ");
        if (!ret) {
            debug_str("err\r\n");
            continue;
        } else {
            debug_32(app_cmd);
            debug_str("\r\n");
        }


        // SEND_OP_COND
        mmc_do_cmd(41, should_try_sdhc ? 0x40300000 : 0x00300000, 0, 0, 3);
        msleep(10);
        uint32_t opcond;
        ret = mmc_finish_r3(&opcond);
        debug_str("opcond ");

        if (!ret) {
            debug_str("err\r\n");
            continue;
        } else {
            debug_32(opcond);
            debug_str("\r\n");
        }

        if (opcond & 0x80000000) {
            debug_str("acmd41 ok!\r\n");
            init_ok = 1;
            is_sdhc = opcond & 0x40000000;
            break;
        }
    }

    if (!init_ok) {
        debug_str("sd card init failed!\r\n");
        panic();
    }

    MMC_CLKRT = 0;

    // ALL_SEND_CID
    mmc_do_cmd(2, 0, 0, 0, 2);
    uint32_t cid[4];
    ret = mmc_finish_r2(cid);
    debug_str("cid ");
    if (!ret) {
        debug_str("err\r\n");
    } else {
        debug_32(cid[0]);
        debug_32(cid[1]);
        debug_32(cid[2]);
        debug_32(cid[3]);
        debug_str("\r\n");
    }

    // SEND_RELATIVE_ADDR
    mmc_do_cmd(3, 0, 0, 0, 1);
    uint32_t rca;
    ret = mmc_finish_r1(3, &rca);
    debug_str("rca ");
    if (!ret) {
        debug_str("err\r\n");
        panic();
    } else {
        debug_32(rca);
        debug_str("\r\n");
    }
    rca &= 0xFFFF0000;

    // SEND_CSD
    mmc_do_cmd(9, rca, 0, 0, 2);
    uint32_t csd[4];
    ret = mmc_finish_r2(csd);
    debug_str("csd ");
    if (!ret) {
        debug_str("err\r\n");
    } else {
        debug_32(csd[0]);
        debug_32(csd[1]);
        debug_32(csd[2]);
        debug_32(csd[3]);
        debug_str("\r\n");
    }

    // SELECT_CARD
    mmc_do_cmd(7, rca, 0, 1, 1);
    uint32_t status;
    ret = mmc_finish_r1(7, &status);
    debug_str("select ");
    if (!ret) {
        debug_str("err\r\n");
        panic();
    } else {
        debug_32(status);
        debug_str("\r\n");
    }

    // SET_BLOCKLEN
    mmc_do_cmd(16, 512, 0, 0, 1);
    ret = mmc_finish_r1(16, &status);
    debug_str("blocklen ");
    if (!ret) {
        debug_str("err\r\n");
    } else {
        debug_32(status);
        debug_str("\r\n");
    }

    while ((status & 0x1F00) != 0x900) {
        // SEND_STATUS
        mmc_do_cmd(13, rca, 0, 0, 1);
        ret = mmc_finish_r1(13, &status);
        debug_str("status ");
        if (!ret) {
            debug_str("err\r\n");
        } else {
            debug_32(status);
            debug_str("\r\n");
        }
    }

    debug_str("sd init all ok!\r\n");

    // Setup FS
    FRESULT fret = f_mount(&fs, "", 1);
    if (fret != FR_OK) {
        debug_str("f_mount failed!\r\n");
        panic();
    }

    int led_state = 1;
    uint32_t *const FLASH = 0;

#ifdef DUMPFLASH
    fret = f_open(&fil, "dump.bin", FA_WRITE | FA_CREATE_ALWAYS);
    if (fret != FR_OK) {
        debug_str("f_open failed!\r\n");
        panic();
    }

    for (unsigned int i = 0; i < 256; i++) {
        set_green(led_state);
        led_state = !led_state;

        unsigned int writelen;
        uint32_t *const this_addr = FLASH + i * 0x10000;
        debug_str("dumping flash at ");
        debug_32((uint32_t)this_addr);
        debug_str("\r\n");
        fret = f_write(&fil, this_addr, 0x40000, &writelen);
        if (fret != FR_OK) {
            debug_str("f_write failed!\r\n");
            panic();
        }
    }

    fret = f_close(&fil);
    if (fret != FR_OK) {
        debug_str("f_close failed!\r\n");
        panic();
    }
#endif

    // Unmount FS
    fret = f_mount(0, "", 0);
    if (fret != FR_OK) {
        debug_str("f_mount unmount failed!\r\n");
        panic();
    }

    debug_str("all done all done!\r\n");

    while (1) {
        set_green(1);
        set_red(1);
        // set_backlight(1);
        msleep(500);
        set_green(0);
        msleep(500);
        set_red(0);
        // set_backlight(0);
        msleep(1000);
    }
}
