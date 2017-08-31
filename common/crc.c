/*
 * crc.c
 *
 *
 * Copyright (c) 2013-2017 Western Digital Corporation or its affiliates.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the copyright holder nor the names of its contributors may not
 *    be used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Author: Jeremy Garff <jeremy.garff@sandisk.com>
 *
 */


#include <stdio.h>
#include <string.h>
#include <stdint.h>


#define CRC32_POLY          0xedb88320UL
#define CRC16_POLY          0x1021
#define CRC8_POLY           0x107


/*
 * The following table is only referenced when not compiled with -DCRC8_SLOW.  Assuming
 * building with compiler flags that only link referenced symbols, the following will
 * not end up in executables where it's not needed.
 */
static const uint8_t crc8_fast_table[] = {
    0x00, 0x07, 0x0e, 0x09, 0x1c, 0x1b, 0x12, 0x15, 0x38, 0x3f, 0x36, 0x31, 0x24, 0x23, 0x2a, 0x2d,
    0x70, 0x77, 0x7e, 0x79, 0x6c, 0x6b, 0x62, 0x65, 0x48, 0x4f, 0x46, 0x41, 0x54, 0x53, 0x5a, 0x5d,
    0xe0, 0xe7, 0xee, 0xe9, 0xfc, 0xfb, 0xf2, 0xf5, 0xd8, 0xdf, 0xd6, 0xd1, 0xc4, 0xc3, 0xca, 0xcd,
    0x90, 0x97, 0x9e, 0x99, 0x8c, 0x8b, 0x82, 0x85, 0xa8, 0xaf, 0xa6, 0xa1, 0xb4, 0xb3, 0xba, 0xbd,
    0xc7, 0xc0, 0xc9, 0xce, 0xdb, 0xdc, 0xd5, 0xd2, 0xff, 0xf8, 0xf1, 0xf6, 0xe3, 0xe4, 0xed, 0xea,
    0xb7, 0xb0, 0xb9, 0xbe, 0xab, 0xac, 0xa5, 0xa2, 0x8f, 0x88, 0x81, 0x86, 0x93, 0x94, 0x9d, 0x9a,
    0x27, 0x20, 0x29, 0x2e, 0x3b, 0x3c, 0x35, 0x32, 0x1f, 0x18, 0x11, 0x16, 0x03, 0x04, 0x0d, 0x0a,
    0x57, 0x50, 0x59, 0x5e, 0x4b, 0x4c, 0x45, 0x42, 0x6f, 0x68, 0x61, 0x66, 0x73, 0x74, 0x7d, 0x7a,
    0x89, 0x8e, 0x87, 0x80, 0x95, 0x92, 0x9b, 0x9c, 0xb1, 0xb6, 0xbf, 0xb8, 0xad, 0xaa, 0xa3, 0xa4,
    0xf9, 0xfe, 0xf7, 0xf0, 0xe5, 0xe2, 0xeb, 0xec, 0xc1, 0xc6, 0xcf, 0xc8, 0xdd, 0xda, 0xd3, 0xd4,
    0x69, 0x6e, 0x67, 0x60, 0x75, 0x72, 0x7b, 0x7c, 0x51, 0x56, 0x5f, 0x58, 0x4d, 0x4a, 0x43, 0x44,
    0x19, 0x1e, 0x17, 0x10, 0x05, 0x02, 0x0b, 0x0c, 0x21, 0x26, 0x2f, 0x28, 0x3d, 0x3a, 0x33, 0x34,
    0x4e, 0x49, 0x40, 0x47, 0x52, 0x55, 0x5c, 0x5b, 0x76, 0x71, 0x78, 0x7f, 0x6a, 0x6d, 0x64, 0x63,
    0x3e, 0x39, 0x30, 0x37, 0x22, 0x25, 0x2c, 0x2b, 0x06, 0x01, 0x08, 0x0f, 0x1a, 0x1d, 0x14, 0x13,
    0xae, 0xa9, 0xa0, 0xa7, 0xb2, 0xb5, 0xbc, 0xbb, 0x96, 0x91, 0x98, 0x9f, 0x8a, 0x8d, 0x84, 0x83,
    0xde, 0xd9, 0xd0, 0xd7, 0xc2, 0xc5, 0xcc, 0xcb, 0xe6, 0xe1, 0xe8, 0xef, 0xfa, 0xfd, 0xf4, 0xf3,
};


/*
 *  This is the same crc calculation used for ethernet, png, zip, etc..
 *  This function is intentionally slow.  This could be implemented
 *  more efficiently using a table, but because this is to be used on
 *  very memory limited targets, this is just brute force.
 */
uint32_t crc32(uint8_t *data, uint32_t len)
{
    uint32_t i, crc = ~0;

    for (i = 0; i < len; i++)
    {
        uint32_t b = (crc & 0xff) ^ data[i];
        uint8_t j;

        for (j = 0; j < 8; j++)
        {
            if (b & 1)
            {
                b = (b >> 1) ^ CRC32_POLY;
            }
            else
            {
                b >>= 1;
            }
        }

        crc = (crc >> 8) ^ b;
    }

    return ~crc;
}

/*
 *  This is the same crc calculation used for xmodem-crc.
 *  This function is intentionally slow.  This could be implemented
 *  more efficiently using a table, but because this is to be used on
 *  very memory limited targets, this is just brute force.
 */
uint32_t crc16(uint8_t *data, uint32_t len)
{
    uint16_t i, crc = ~0;

    for (i = 0; i < len; i++)
    {
        uint16_t b = (crc & 0xff) ^ data[i];
        uint8_t j;

        for (j = 0; j < 8; j++)
        {
            if (b & 1)
            {
                b = (b >> 1) ^ CRC16_POLY;
            }
            else
            {
                b >>= 1;
            }
        }

        crc = (crc >> 8) ^ b;
    }

    return ~crc;
}

// Used for PEC validation of SMBus packets
uint8_t crc8_next_byte(uint8_t crc, uint8_t data)
{
    uint8_t shift = crc ^ data;
#ifdef CRC8_SLOW
    int i;

    for (i = 0; i < 8; i++)
    {
        if (shift & 0x80)  // Test high bit
        {
            shift <<= 1;
            shift ^= (CRC8_POLY & 0xff);
        }
        else
        {
            shift <<= 1;
        }
    }
#else /* CRC8_SLOW */
    shift = crc8_fast_table[shift];
#endif /* CRC8_SLOW */

    return shift;
}

uint8_t crc8(uint8_t *data, uint32_t len)
{
    uint8_t crc = 0;

    while (len--)
    {
        crc = crc8_next_byte(crc, *data++);
    }

    return crc;
}
