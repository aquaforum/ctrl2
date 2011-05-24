// Source for AVR developed by rwatson: http://www.koders.com/c/fid56DF10C05FA8132E1632F12B233B86FB9DE74B7B.aspx
// Ported to ARM by cherep22

#ifndef ds18b20_h
#define ds18b20_h

//----- Include Files ---------------------------------------------------------
//#include "global.h"

//----- Defines ---------------------------------------------------------------
#define ds18b20_rev					3

// family code
#define DS18S20_FAMILY				0x10
#define DS18B20_FAMILY				0x28

// function commands
#define DS18B20_CONVERT_TEMP		0x44
#define DS18B20_WRITE_SCRATCHPAD	0x4E
#define DS18B20_READ_SCRATCHPAD		0xBE
#define DS18B20_COPY_SCRATCHPAD		0x48
#define DS18B20_RECALL_E2			0xB8
#define DS18B20_READ_POWER			0xB4

// resolution min and max
#define DS18B20_RES_MIN				9
#define DS18B20_RES_MAX				12

// no alarm values for high and low
#define DS18B20_NO_ALARM_LOW		-56		// min temp read is -55C
#define DS18B20_NO_ALARM_HIGH		126		// max temp read is 125C

//----- Functions ---------------------------------------------------------------

#ifdef	__cplusplus
extern "C" {
#endif

// ds18b20Init()
//     initializes the dallas 1-wire bus
void ds18b20Init(void);

// ds18b20Setup
//     Sets up the device
//     The parameters are the rom id of the device,
//     the resolution [9-12], and the low and high alarm values.
//     If no low and/or high alarm is desired, use the values -55 and/or 126
//     Returns either the corresponding error or DALLAS_NO_ERROR
u08 ds18b20Setup(dallas_rom_id_T* rom_id, u08 resolution, s08 alarm_low, s08 alarm_high);

// ds18b20GetResolution
//     Get device resolution
//     The parameters are the rom id of the device,
//     Output parameter esolution [9-12] is set.
//     Returns either the corresponding error or DALLAS_NO_ERROR
u08 ds18b20GetResolution(dallas_rom_id_T* rom_id, u08 *resolution);

// ds18x20CheckAddress
//     Check that rom id has valid family code (DS18B20 or DS18S20)
//     Returns the corresponding error or DALLAS_NO_ERROR
u08 ds18x20CheckAddress(dallas_rom_id_T *rom_id);

// ds18b20Start()
//     Start the conversion for the given device
//     Returns either the corresponding error or DALLAS_NO_ERROR
u08 ds18b20Start(dallas_rom_id_T* rom_id);

// ds18b20Result()
//     Gets the result of the conversion and stores it in *result
//     Returns either the corresponding error or DALLAS_NO_ERROR
u08 ds18b20Result(dallas_rom_id_T* rom_id, u16 *result);

// ds18b20StartAndResult();
//     1-step command to start the conversion and store the result in *result
//     The conversion takes some time to do, so it can be more efficient
//     to do the 1-step commands Start() and Result()
//     Returns either the corresponding error or DALLAS_NO_ERROR
u08 ds18b20StartAndResult(dallas_rom_id_T* rom_id, u16 *result);

// ds18b20Print()
//     Does a formatted print on the given resultat the given resolution: +xx x/xx
//void ds18b20Print(u16 result, u08 resolution);

#ifdef	__cplusplus
};
#endif

#endif
