// Source for AVR developed by rwatson: http://www.koders.com/c/fid5B299A66C1ABB77ACB5B181C2D491A2FE7496991.aspx
// Ported to ARM by cherep22

//----- Include Files ---------------------------------------------------------
#include <string.h>			// include string support
#include <stdio.h>
#include "dallas.h"			// include dallas support
#include "ds2450.h"			// include ds2450 support
#include "crc.h"

//----- Functions ---------------------------------------------------------------

#ifdef	__cplusplus
extern "C" {
#endif

static void ds2450_strupr(u08 *s)
{
	if (*s >= 'a' && *s <= 'z')
		*s += 'A' - 'a';
}

/*--------------------------------------------------------------------------
 * ds2450Chan2Addr: converts the channel to the address in RAM
 * input........... channel - the channel to get the address for [A-D]
 *                  page - the page in RAM that we are dealing with
 *                  address - where the address is stored
 * returns......... the corresponding error or DALLAS_NO_ERROR
 *-------------------------------------------------------------------------*/
static u08 ds2450Chan2Addr(u08 channel, u08 page, u16 *address);

void ds2450Init(void)
{
	// initialize the dallas 1-wire
	// dallasInit();
}

u08 ds2450Setup(dallas_rom_id_T* rom_id, u08 channel, u08 resolution, u08 range)
{
	u08 data[2];
	u16 address;

	// check resolution
	if (resolution > DS2450_RES_MAX)
		return DALLAS_RESOLUTION_ERROR;

	// check address
	DALLAS_CHECK(dallasAddressCheck(rom_id, DS2450_FAMILY));

	// get address
	ds2450_strupr(&channel);
	DALLAS_CHECK(ds2450Chan2Addr(channel, DS2450_SETUP_PAGE, &address)); 	//find starting address

	// convert to valid resolution - 16 bits = 0x00
	if (resolution == 16)
		resolution = 0x00;

	// read in current digital output settings
	DALLAS_CHECK(dallasReadRAM(rom_id, address, 1, data));

	data[0] = (data[0] & 0xF0) | resolution;	// maintain digital output portion and add new resolution
	data[1] = (data[1] & 0xFE) | range;			// maintain alarm states and add new range

	// actually write config, handles CRC too
	DALLAS_CHECK(dallasWriteRAM(rom_id, address, 2, data));

	// Normally, the DS2450 is designed to run off of parasite power from the data line
	// Typically the master (us) strongly pulls high long enough to power the conversion, so
	// there is inherintly a long () delay introduced. Since the A2D code is designed to
	// work for devices that use external power, we can elliminate this delay by writting
	// the following byte per the DS2450 datasheet.
	data[0] = DS2450_VCC_FLAG;
	DALLAS_CHECK(dallasWriteRAM(rom_id, DS2450_VCC_ADDR, 1, &data[0]));

	// verify the data
	DALLAS_CHECK(dallasReadRAM(rom_id, address, 2, data));

	if ((data[0] & 0x0F) != resolution)
		return DALLAS_VERIFY_ERROR;
	if ((data[1] & 0x01) != range)
		return DALLAS_VERIFY_ERROR;

	return DALLAS_NO_ERROR;
}

u08 ds2450Start(dallas_rom_id_T* rom_id, u08 channel)
{
	u08 mask;
	u16 crc = 0;
	
	DALLAS_CHECK(dallasAddressCheck(rom_id, DS2450_FAMILY));	// check address

	ds2450_strupr(&channel);			// make sure the channel is a capital letter
	channel -= 'A';				// convert to integer 0 to 3

	// make sure channel is a valid value
	if (channel > 3)
		return DALLAS_INVALID_CHANNEL;

	// shift over to construct input select mask
	mask = 0x01 << channel;

	dallasBufferEnabled(1);

	// reset and select node
	DALLAS_CHECK(dallasMatchROM(rom_id));

	// send convert command
	dallasWriteByte(DS2450_CONVERT);
	crc = crc16_update(crc, DS2450_CONVERT);
	// input select mask
	dallasWriteByte(mask);
	crc = crc16_update(crc, mask);
	// shift over some more for "read-out" control
	mask = mask << channel;
	
	// set coresponding output buffer to zero	
	dallasWriteByte(mask);
	crc = crc16_update(crc, mask);

	dallasBufferRead(2);
	// we must read 2byte CRC16 to start the conversion:
	crc ^= dallasReadByte();
	crc ^= dallasReadByte() << 8;

	dallasBufferEnabled(0);

	//replace with explicit CRC posibilities lookup table
	if (crc)
		return DALLAS_CRC_ERROR;        //if CRC is not zero, no one is paying attention

	return DALLAS_NO_ERROR;
}

u08 ds2450Result(dallas_rom_id_T* rom_id, u08 channel, u16* result)
{
	u08 data[2];
	u16 address;
	u08 resolution;

	DALLAS_CHECK(dallasAddressCheck(rom_id, DS2450_FAMILY));				// check address

	ds2450_strupr(&channel);
	DALLAS_CHECK(ds2450Chan2Addr(channel, DS2450_DATA_PAGE, &address));		// get the RAM address for the data for the channel
	DALLAS_CHECK(dallasReadRAM(rom_id, address, 2, data));					// read the RAM from the device to get the data
	// get the address for the setup for the channel
	DALLAS_CHECK(ds2450Chan2Addr(channel, DS2450_SETUP_PAGE, &address)); 	//find starting address
	DALLAS_CHECK(dallasReadRAM(rom_id, address, 1, &resolution));			// read the RAM from the device to get the resolution

	// get the resultion part of the data
	resolution &=0x0F;

	// store the result by combining the 2 bytes
	// the result's MSB is always the same, so we may need to
	// shift the data over so that the LSB is at the first bit
	*result = 0;
	//*result = (((u16)data[1] << 8) | data[0]) >> (16 - resolution);
	*result = (((u16)data[1] << 8) | data[0]);

	return DALLAS_NO_ERROR;
}

u08 ds2450StartAndResult(dallas_rom_id_T* rom_id, u08 channel, u16 *result)
{
	DALLAS_CHECK(ds2450Start(rom_id, channel));	// start conversion
	dallasWaitUntilDone();						// wait till conversion done
	return ds2450Result(rom_id, channel, result);
}

u08 ds2450SetupAll(dallas_rom_id_T* rom_id, u08 resolution, u08 range)
{
	u08 res[4] = {resolution, resolution, resolution, resolution};
	u08 rng[4] = {range, range, range, range};
	return ds2450WriteAllSettings(rom_id, res, rng, 0);
}

u08 ds2450WriteAllSettings(dallas_rom_id_T* rom_id, u08 *resolution, u08 *range, u08 *digital_output)
{
	u08 i;
	u08 data[8];
	u16 address;

	DALLAS_CHECK(dallasAddressCheck(rom_id, DS2450_FAMILY));	// check address
	for (i = 0; i < 4; ++i) {
		if (range) {
			if (range[i] != DS2450_RANGE_2V && range[i] != DS2450_RANGE_5V && range[i] != DS2450_RANGE_DO_NOT_CHANGE)
				return DALLAS_RANGE_ERROR;
		}
		if (resolution) {
			if (resolution[i] > DS2450_RES_MAX && resolution[i] != DS2450_RES_DO_NOT_CHANGE)
				return DALLAS_RESOLUTION_ERROR;
		}
		if (digital_output) {
			if (digital_output[i] != DS2450_OUTPUT_LOW && digital_output[i] != DS2450_OUTPUT_HIGH && digital_output[i] != DS2450_OUTPUT_DO_NOT_CHANGE)
				return DALLAS_RESOLUTION_ERROR;
		}
	}
	
	DALLAS_CHECK(ds2450Chan2Addr('A', DS2450_SETUP_PAGE, &address));	// get address - start with channel A
	DALLAS_CHECK(dallasReadRAM(rom_id, address, 8, data));				// read in current settings so we can extract digital part

	// build up config data to write - increment by 2 b/c two bytes per channel
	for (i = 0; i < 8; i += 2) { 
		if (resolution && resolution[i >> 1] != DS2450_RES_DO_NOT_CHANGE) {
			data[i] &= 0xF0;						// extract digital output portion
			data[i] |= resolution[i >> 1] & 0x0F;	// convert to valid resolution - 16 bits = 0x00
		}
		if (digital_output && digital_output[i >> 1] != DS2450_OUTPUT_DO_NOT_CHANGE) {
			data[i] &= 0x3F;
			data[i] |= (digital_output[i >> 1] == DS2450_OUTPUT_LOW ? 0x80 : 0x00);
		}
		if (range && range[i >> 1] != DS2450_RANGE_DO_NOT_CHANGE) {
			data[i+1] &= 0xFE;
			data[i+1] |= range[i >> 1];
		}
	}

	// actually write config - handles CRC too
	DALLAS_CHECK(dallasWriteRAM(rom_id, address, 8, data));

	// Normally, the DS2450 is designed to run off of parasite power from the data line
	// Typically the master (us) strongly pulls high long enough to power the conversion, so
	// there is inherintly a long () delay introduced. Since the A2D code is designed to
	// work for devices that use external power, we can elliminate this delay by writting
	// the following byte per the DS2450 datasheet.
	data[0] = DS2450_VCC_FLAG;
	DALLAS_CHECK(dallasWriteRAM(rom_id, DS2450_VCC_ADDR, 1, &data[0]));

	return DALLAS_NO_ERROR;
}

u08 ds2450ReadAllSettings(dallas_rom_id_T* rom_id, u08 *resolution, u08 *range, u08 *digital_output)
{
	u08 i;
	u08 data[8];
	u16 address;

	DALLAS_CHECK(dallasAddressCheck(rom_id, DS2450_FAMILY));			// check address
	DALLAS_CHECK(ds2450Chan2Addr('A', DS2450_SETUP_PAGE, &address));	// get address - start with channel A
	DALLAS_CHECK(dallasReadRAM(rom_id, address, 8, data));				// read in current settings so we can extract digital part

	// analyze config data - increment by 2 b/c two bytes per channel
	for(i=0;i<8;i+= 2) { 
		if (resolution) {
			resolution[i >> 1] = data[i] & 0x0F;
			if (resolution[i >> 1] == 0)								// convert to conventional resolution - 0x00 = 16 bits
				resolution[i >> 1] = 16;
		}
		if (digital_output) {
			digital_output[i >> 1] = ((data[i] & 0xC0) == 0x80 ? DS2450_OUTPUT_LOW : DS2450_OUTPUT_HIGH);
		}
		if (range) {
			range[i >> 1] = data[i+1] & 0x01;
		}
	}

	return DALLAS_NO_ERROR;
}

u08 ds2450StartAll(dallas_rom_id_T* rom_id)
{
	u16 crc = 0;

	DALLAS_CHECK(dallasAddressCheck(rom_id, DS2450_FAMILY));	// check address
	dallasBufferEnabled(1);
	DALLAS_CHECK(dallasMatchROM(rom_id));						// reset and select node

	dallasWriteByte(DS2450_CONVERT);				// send convert command
	dallasWriteByte(DS2450_CONVERT_ALL4_MASK);  	// select all 4 inputs
	dallasWriteByte(DS2450_CLEAR_ALL4_MASK);		// set all output buffers to zero

	crc = crc16_update(crc, DS2450_CONVERT);
	crc = crc16_update(crc, DS2450_CONVERT_ALL4_MASK);
	crc = crc16_update(crc, DS2450_CLEAR_ALL4_MASK);

	dallasBufferRead(2);
	crc = ~crc;
	// we must read 2byte CRC16 to start the conversion:
	crc ^= dallasReadByte();
	crc ^= dallasReadByte() << 8;
	dallasBufferEnabled(0);

	if (crc)
		return DALLAS_CRC_ERROR;

	return DALLAS_NO_ERROR;
}

u08 ds2450ResultAll(dallas_rom_id_T* rom_id, u16 result[4])
{
	//const u08 bytes_to_read = 10;		// read 10bytes = 2/ch*4ch + CRC
	u08 bytes_to_read = 10;
	u08 i;
	u08 error;
	u08 data[10];
	u08 resolution[10];
	u16 address;

	DALLAS_CHECK(dallasAddressCheck(rom_id, DS2450_FAMILY));			// check address
	DALLAS_CHECK(ds2450Chan2Addr('A', DS2450_DATA_PAGE, &address));		// start address with channel A
	DALLAS_CHECK(dallasReadRAM(rom_id, address, bytes_to_read, data));	// read the conversion data

	//FUTURE: do a real CRC16 check

	DALLAS_CHECK(ds2450Chan2Addr('A', DS2450_SETUP_PAGE, &address));	// start address with channel A
	DALLAS_CHECK(dallasReadRAM(rom_id, address, bytes_to_read, resolution));	// read the resolution data

	// check crc?

	// store the result by combining the 2 bytes
	// the result's MSB is always the same, so we may need to
	// shift the data over so that the LSB is at the first bit
	error=0;
	for(i=0;i<8;i+=2)
	{
		resolution[i] &= 0x0F;
		if (!resolution[i])
			resolution[i] = 16;

		result[error] = 0;
		//result[error] = (((u16)data[i+1] << 8) | data[i]) >> (16 - resolution[i]);
		result[error] = (((u16)data[i+1] << 8) | data[i]);
		error++;
	}

	return DALLAS_NO_ERROR;
}

u08 ds2450StartAndResultAll(dallas_rom_id_T* rom_id, u16 result[4])
{
	DALLAS_CHECK(ds2450StartAll(rom_id));	// start Conversion
	dallasWaitUntilDone();					// wait until conversion done
	return ds2450ResultAll(rom_id, result);	// return any error - results passed by reference
}

//void ds2450Print(u16 result, u08 range)
//{
//	u16 vscale;
//
//	rprintfProgStrM(" 0x");
//	rprintfu16(result);
//	rprintf("  ");
//	if(range)
//		vscale = 12800;
//	else
//		vscale = 25600;
//
//	//rprintfNum(10, 4, TRUE , ' ', result/vscale);
//	rprintf("%4d", result/vscale);
//	rprintf(".");
//	//rprintfNum(10, 4, FALSE, '0', (((u32)(result%vscale))*10000)/vscale );
//	rprintf("%04u", (((u32)(result%vscale))*10000)/vscale);
//	rprintfProgStrM(" Volts");
//}

u08 ds2450DigitalOut(dallas_rom_id_T* rom_id, u08 channel, dallas_a2d_out_T state)
{
	u08 old_resolution;
	u16 address;

	DALLAS_CHECK(dallasAddressCheck(rom_id, DS2450_FAMILY));				// check address
	DALLAS_CHECK(ds2450Chan2Addr(channel, DS2450_SETUP_PAGE, &address));	// get the address for the channel in the setup page
	DALLAS_CHECK(dallasReadRAM(rom_id, address, 1, &old_resolution));		// read in current resolution
	
	// extract resolution portion
	old_resolution &= 0x0F;

	// write new setup byte
	state |= old_resolution;
	DALLAS_CHECK(dallasWriteRAM(rom_id, address, 1, ((u08*)&state)));

	return DALLAS_NO_ERROR;
}

static u08 ds2450Chan2Addr(u08 channel, u08 page, u16 *address)
{
	ds2450_strupr(&channel);		// make sure the channel is a capital letter
	channel -= 'A';			//convert to integer 0 to 3 and check to see if it is valid
	if (channel > 3)
		return DALLAS_INVALID_CHANNEL;

	// use corresponding memory address
	*address = (channel<<1) + page;			// channel<<1 == channel*2, but faster

	return DALLAS_NO_ERROR;
}

#ifdef	__cplusplus
};
#endif
