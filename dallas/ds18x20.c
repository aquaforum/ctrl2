// Source for AVR developed by rwatson: http://www.koders.com/c/fidD3FC654DB8F433BB94F6932E2C2A10451B5F1F99.aspx
// Ported to ARM by cherep22

//----- Include Files ---------------------------------------------------------
//#include "uart.h"			// include uart function library
#include "dallas.h"			// include dallas support
#include "ds18x20.h"		// include ds18b20 support
#include "crc.h"
//#include <windows.h>

typedef union ds18x20_scratch_pad_U {
	struct {
		u08 t_low;
		u08 t_high;
		u08 alarm_high;
		u08 alarm_low;
		union {
			struct {
				u08 resolution;
				u08 reserved_1;
				u08 reserved_2;
				u08 reserved_3;
			} ds18b20;
			struct {
				u08 reserved_1;
				u08 reserved_2;
				u08 count_remain;
				u08 count_per_c;
			} ds18s20;
		} config;
	} data;
	u08 byte[8];
} ds18x20_scratch_pad_T;

#ifdef	__cplusplus
extern "C" {
#endif

void ds18b20Init(void)
{
	// initialize the 1-wire
	// dallasInit();
}

static u08 ds18x20ReadScratchPad(dallas_rom_id_T* rom_id, ds18x20_scratch_pad_T *scratch_pad)
{
	u08 i;
	u08 crc = 0;

	dallasBufferEnabled(1);
	DALLAS_CHECK(dallasMatchROM(rom_id));

	// start reading at address 0x00
	dallasWriteByte(DS18B20_READ_SCRATCHPAD);
	dallasBufferRead(sizeof(ds18x20_scratch_pad_T) + 1);
	for (i = 0; i < sizeof(ds18x20_scratch_pad_T); ++i)
	{
		scratch_pad->byte[i] = dallasReadByte();		// 0x00-0x07, bytes 0-7
		crc = crc8_update(crc, scratch_pad->byte[i]);
	}
	i = dallasReadByte();
	crc = crc8_update(crc, i); 			// 0x08, crc
	dallasBufferEnabled(0);
	if (crc != 0)
		return DALLAS_CRC_ERROR;
	return DALLAS_NO_ERROR;
}

u08 ds18b20GetResolution(dallas_rom_id_T* rom_id, u08 *resolution)
{
	ds18x20_scratch_pad_T scratch_pad;

	// check address
	DALLAS_CHECK(dallasAddressCheck(rom_id, DS18B20_FAMILY));

	// reset and select
	DALLAS_CHECK(dallasMatchROM(rom_id));

	// read scratch pad and check CRC
	DALLAS_CHECK(ds18x20ReadScratchPad(rom_id, &scratch_pad));

	*resolution = (scratch_pad.data.config.ds18b20.resolution >> 5) & 0x03;

	return DALLAS_NO_ERROR;
}

u08 ds18b20Setup(dallas_rom_id_T* rom_id, u08 resolution, s08 alarm_low, s08 alarm_high)
{
	ds18x20_scratch_pad_T scratch_pad;

	// check resolution
	if ((resolution < DS18B20_RES_MIN) || (resolution > DS18B20_RES_MAX))
		return DALLAS_RESOLUTION_ERROR;

	// check address
	DALLAS_CHECK(ds18x20CheckAddress(rom_id));

	// reset and select
	DALLAS_CHECK(dallasMatchROM(rom_id));

	// starts writting at address 0x02, T_H
	dallasWriteByte(DS18B20_WRITE_SCRATCHPAD);
	dallasWriteByte(alarm_high);
	dallasWriteByte(alarm_low);

	if (rom_id->byte[0] == DS18B20_FAMILY)
	{
		// convert resolution to bitmask
		// valid value are 9-12 encoded as 0-4, resolution stored in bits 5&6 and bits 0-4 are always one
		resolution = ((resolution - 9) << 5) | 0x1F;
		dallasWriteByte(resolution);
	}

	// read scratch pad and check CRC
	DALLAS_CHECK(ds18x20ReadScratchPad(rom_id, &scratch_pad));

	// verify the data
	if (scratch_pad.data.alarm_high != (u08)alarm_high)
		return DALLAS_VERIFY_ERROR;
	if (scratch_pad.data.alarm_low != (u08)alarm_low)
		return DALLAS_VERIFY_ERROR;

	if (rom_id->byte[0] == DS18B20_FAMILY)
		if (scratch_pad.data.config.ds18b20.resolution != resolution)
			return DALLAS_VERIFY_ERROR;

	return DALLAS_NO_ERROR;
}

u08 ds18x20CheckAddress(dallas_rom_id_T *rom_id)
{
	u08 error;
	error = dallasAddressCheck(rom_id, DS18B20_FAMILY);
	if (error != DALLAS_NO_ERROR)
		error = dallasAddressCheck(rom_id, DS18S20_FAMILY);
	return error;
}

u08 ds18b20Start(dallas_rom_id_T* rom_id)
{
	dallasBufferEnabled(1);
	if (rom_id)
	{
		DALLAS_CHECK(ds18x20CheckAddress(rom_id));
		DALLAS_CHECK(dallasMatchROM(rom_id));
	}
	else
	{
		DALLAS_CHECK(dallasSkipROM());
	}
	
	// send convert command
	dallasWriteByte(DS18B20_CONVERT_TEMP);
	dallasBufferEnabled(0);
	
	return DALLAS_NO_ERROR;
}

u08 ds18b20Result(dallas_rom_id_T* rom_id, u16 *result)
{
	ds18x20_scratch_pad_T scratch_pad;
	DALLAS_CHECK(ds18x20CheckAddress(rom_id));
	DALLAS_CHECK(ds18x20ReadScratchPad(rom_id, &scratch_pad));
	*result = scratch_pad.data.t_high << 8 | scratch_pad.data.t_low;
	if (rom_id->byte[DALLAS_FAMILY_IDX] == DS18S20_FAMILY)
		*result <<= 3;
	return DALLAS_NO_ERROR;
}

u08 ds18b20StartAndResult(dallas_rom_id_T* rom_id, u16 *result)
{
	DALLAS_CHECK(ds18b20Start(rom_id));		// start
	dallasWaitUntilDone();					// wait
	return ds18b20Result(rom_id, result);	// return any errors - results passed by reference
}

//void ds18b20Print(u16 result, u08 resolution)
//{
//	// print raw value
//	rprintfProgStrM(" 0x");
//	rprintfu16(result);
//	rprintfChar(' ');
//
//	// print real temp
//	//rprintfNum(10, 4, TRUE , ' ', result>>4);
//	rprintf("%4d", result>>4);
//	rprintf(".");
//	//rprintfNum(10, 4, FALSE, '0', (10000*((u32)(result&0x000F)))/16 );
//	rprintf("%04u", (10000*((u32)(result&0x000F)))/16);
//	rprintfProgStrM(" C");
//	// rprintfCRLF();
//}

#ifdef	__cplusplus
};
#endif
