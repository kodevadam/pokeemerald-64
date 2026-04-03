/*
 * tools/n64crc.c — N64 ROM CRC fixer
 *
 * Calculates and writes the two CRC checksums at offsets 0x10 and 0x14
 * in an N64 ROM header, using the standard N64 CRC algorithm.
 *
 * Usage: n64crc <romfile.v64>
 *
 * Supports both .z64 (big-endian) and .v64 (byte-swapped) ROMs,
 * detected automatically from the first byte of the ROM header.
 *
 * CRC algorithm based on the N64 IROM CRC routine (CIC NUS-6101/6102).
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define HEADER_SIZE   0x40
#define CRC_START     0x00001000   /* CRC computed from this offset */
#define CRC_LENGTH    0x00100000   /* over this many bytes (1 MB)   */

static uint32_t bswap32(uint32_t x)
{
    return ((x & 0xFF000000) >> 24) |
           ((x & 0x00FF0000) >>  8) |
           ((x & 0x0000FF00) <<  8) |
           ((x & 0x000000FF) << 24);
}

/* Rotate-left 32-bit */
static inline uint32_t rotl32(uint32_t v, int n)
{
    return (v << n) | (v >> (32 - n));
}

/* Compute N64 CRC (CIC-6102 algorithm) */
static void n64_crc(const uint8_t *rom, size_t romsize,
                    uint32_t *crc1_out, uint32_t *crc2_out)
{
    /* CIC-6102 seed */
    const uint32_t SEED = 0xF8CA4DDC;

    uint32_t t1 = SEED, t2 = SEED, t3 = SEED;
    uint32_t t4 = SEED, t5 = SEED, t6 = SEED;

    for (size_t i = CRC_START; i < CRC_START + CRC_LENGTH; i += 4) {
        uint32_t d;
        if (i + 4 > romsize) {
            d = 0;
        } else {
            /* ROM bytes are big-endian words */
            d = ((uint32_t)rom[i]   << 24) |
                ((uint32_t)rom[i+1] << 16) |
                ((uint32_t)rom[i+2] <<  8) |
                ((uint32_t)rom[i+3]);
        }

        if ((t6 + d) < t6) t4++;
        t6  = t6 + d;
        t3 ^= d;
        uint32_t r = rotl32(d, (d & 0x1F));
        t5  = t5 + r;
        if (t2 > d)
            t2 ^= r;
        else
            t2 ^= t6 ^ d;
        t1 += (t5 ^ d);
    }

    *crc1_out = t6 ^ t4 ^ t3;
    *crc2_out = t5 ^ t2 ^ t1;
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <rom.v64|rom.z64>\n", argv[0]);
        return 1;
    }

    FILE *f = fopen(argv[1], "r+b");
    if (!f) { perror(argv[1]); return 1; }

    /* Read entire ROM */
    fseek(f, 0, SEEK_END);
    long romsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (romsize < (long)(CRC_START + CRC_LENGTH)) {
        fprintf(stderr, "ROM too small (need at least 0x101000 bytes)\n");
        fclose(f);
        return 1;
    }

    uint8_t *rom = malloc(romsize);
    if (!rom) { perror("malloc"); fclose(f); return 1; }
    if (fread(rom, 1, romsize, f) != (size_t)romsize) {
        perror("fread"); free(rom); fclose(f); return 1;
    }

    /* Detect byte order from magic byte */
    int is_v64 = (rom[0] == 0x37); /* v64 first byte */
    int is_z64 = (rom[0] == 0x80); /* z64 first byte */
    (void)is_z64;

    /* If .v64 (byte-swapped), swap to canonical .z64 for CRC computation */
    uint8_t *work = rom;
    if (is_v64) {
        work = malloc(romsize);
        if (!work) { perror("malloc"); free(rom); fclose(f); return 1; }
        /* v64 swaps every pair of bytes */
        for (long i = 0; i + 1 < romsize; i += 2) {
            work[i]   = rom[i+1];
            work[i+1] = rom[i];
        }
    }

    uint32_t crc1, crc2;
    n64_crc(work, romsize, &crc1, &crc2);

    printf("CRC1: %08X  CRC2: %08X\n", crc1, crc2);

    /* Write CRCs back into the ROM header at 0x10 and 0x14 (big-endian) */
    if (is_v64) {
        /* Write into byte-swapped positions (v64 format) */
        /* Offset 0x10 big-endian = bytes [b3,b2,b1,b0] → v64 = [b2,b3,b0,b1] */
        rom[0x10] = (crc1 >> 16) & 0xFF;
        rom[0x11] = (crc1 >> 24) & 0xFF;
        rom[0x12] = (crc1      ) & 0xFF;
        rom[0x13] = (crc1 >>  8) & 0xFF;
        rom[0x14] = (crc2 >> 16) & 0xFF;
        rom[0x15] = (crc2 >> 24) & 0xFF;
        rom[0x16] = (crc2      ) & 0xFF;
        rom[0x17] = (crc2 >>  8) & 0xFF;
        free(work);
    } else {
        /* .z64: big-endian as-is */
        rom[0x10] = (crc1 >> 24) & 0xFF;
        rom[0x11] = (crc1 >> 16) & 0xFF;
        rom[0x12] = (crc1 >>  8) & 0xFF;
        rom[0x13] = (crc1      ) & 0xFF;
        rom[0x14] = (crc2 >> 24) & 0xFF;
        rom[0x15] = (crc2 >> 16) & 0xFF;
        rom[0x16] = (crc2 >>  8) & 0xFF;
        rom[0x17] = (crc2      ) & 0xFF;
    }

    fseek(f, 0, SEEK_SET);
    fwrite(rom, 1, romsize, f);
    fclose(f);
    free(rom);

    printf("CRC patched into %s\n", argv[1]);
    return 0;
}
