#include "crc.h"

#define CRC8INIT	0x00
#define CRC8POLY	0x8C              //0x8C = X^8+X^5+X^4+X^0

#define CRC16INIT	0x0000
#define CRC16POLY	0xA001            //0xA001 = X^16+X^15+X^2+X^0

u08 crc8(u08 *data, u16 size)
{
    u08 crc;
    u16 i;
    crc = CRC8INIT;
    for (i = 0; i < size; ++i)
        crc = crc8_update(crc, data[i]);
    return crc;
}

u08 crc8_update(u08 crc, u08 next_byte)
{
    u16 i;
    crc ^= next_byte;
    for (i = 0; i < 8; ++i)
		crc = (crc >> 1) ^ ((crc & 1) ? CRC8POLY : 0);
    return crc;
}

u16 crc16(u08 *data, u16 size)
{
    u16 crc;
    u16 i;
    crc = CRC16INIT;
    for (i = 0; i < size; ++i)
        crc = crc16_update(crc, data[i]);
    return crc;
}

u16 crc16_update(u16 crc, u08 next_byte)
{
    u16 i;
    crc ^= next_byte;
    for (i = 0; i < 8; ++i)
		crc = (crc >> 1) ^ ((crc & 1) ? CRC16POLY : 0);
    return crc;
}
