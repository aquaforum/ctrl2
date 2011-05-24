#ifndef _CRC_H_
#define _CRC_H_

#include "types.h"

u08 crc8(u08 *data, u16 size);
u08 crc8_update(u08 crc, u08 next_byte);
u16 crc16(u08 *data, u16 size);
u16 crc16_update(u16 crc, u08 next_byte);

#endif
