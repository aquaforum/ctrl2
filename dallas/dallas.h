// Source for AVR developed by rwatson: http://www.koders.com/c/fidFAF234B4F73F8BB87FA20A4588F5A93442F1D494.aspx
// Ported to ARM by cherep22


#ifndef dallas_h
#define dallas_h

//----- Include Files ---------------------------------------------------------
#include "types.h"

//----- Defines ---------------------------------------------------------------
#define dallas_rev					6

#define DS_UNKNOWN_FAMILY 0xFF

// indexes of named bytes in the
// dallas address array
#define DALLAS_FAMILY_IDX			0			// family code
#define DALLAS_A0_IDX				1
#define DALLAS_A1_IDX				2
#define DALLAS_A2_IDX				3
#define DALLAS_A3_IDX				4
#define DALLAS_A4_IDX				5
#define DALLAS_A5_IDX				6
#define DALLAS_CRC_IDX				7			// crc code

// dallas return error codes
#define DALLAS_NO_ERROR				0			// all is well
#define DALLAS_NO_PRESENCE			'g'			// presence wasn't detected
#define DALLAS_VERIFY_ERROR			'v'			// setup verification failed
#define DALLAS_ADDRESS_ERROR		'a'			// bad address for command: either wrong family or bad CRC
#define DALLAS_CRC_ERROR			'c'			// data/return value fails CRC check
#define DALLAS_DEVICE_ERROR			'd'			// device not responding
#define DALLAS_NULL_POINTER			'p'			// dallas function passed a NULL pointer
#define DALLAS_ZERO_LEN				'z'			// ReadRAM or WriteRAM asked to read/write zero bytes
#define DALLAS_BUS_ERROR			'b'			// Bus hardware error. (wrong voltage) Possible causes:
													// - lack of pullup Resistor
													// - Other master transmitting (Dallas is not multi-master)
													// - Device failure
#define DALLAS_OS_ERROR				'O'			// OS error

// ds2450 and ds18b20 errors
// defined here to work with PrintError
#define DALLAS_RESOLUTION_ERROR		'r'			// invalid resolution specified in Dallas function
#define DALLAS_RANGE_ERROR			'e'			// invalid range specified in Dallas function
#define DALLAS_INVALID_CHANNEL		'i'			// channel outside the range 'A' to 'D'
#define DALLAS_FORMAT_ERROR			'f'			// results are not in a valid format (temp sensor)

// ROM commands
#define DALLAS_READ_ROM				0x33
#define DALLAS_MATCH_ROM			0x55
#define DALLAS_SKIP_ROM				0xCC
#define DALLAS_SEARCH_ROM			0xF0
#define DALLAS_CONDITIONAL_SEARCH	0xEC
#define DALLAS_READ_MEMORY			0xAA
#define DALLAS_WRITE_MEMORY			0x55


#define DALLAS_CHECK(x) { u08 error; error = (x); if (error != DALLAS_NO_ERROR) { dallasBufferEnabled(0); return error; } }
#define DALLAS_CHECK_WITH_DUMP(x) { u08 error; error = (x); if (error != DALLAS_NO_ERROR) { dallasPrintError(error); dallasBufferEnabled(0); return error; } }

// #define _DALLAS_BUFFER_ENABLED_

//----- Typedefs --------------------------------------------------------------

// typedef for the rom IDs
// done so we can access the entire id or each individual byte
typedef union dallas_rom_id_U
{
	unsigned long long id;
	u08 byte[8];
} dallas_rom_id_T;

//----- Functions ---------------------------------------------------------------

#ifdef	__cplusplus
extern "C" {
#endif

// dallasInit()
//     Initializes the Dallas 1-wire Bus
//     Currently this function does nothing
u08 dallasInit(char*);

void dallasDeinit(void);

// dallasReset()
//     performs a reset on the 1-wire bus
//     returns DALLAS_NO_ERROR, DALLAS_NO_PRESENCE or DALLAS_BUS_ERROR
u08  dallasReset(void);

// dallasReadBit()
//     reads a bit from the 1-wire bus and returns this bit
//     note: global interupts are not disabled in this function
//           if using this function, use cli() and sei() before and after
#define dallasReadBit()		(dallasWriteBit(1))

// dallasWriteBit()
//     writes the passed in bit to the 1-wire bus
//     note: global interupts are not disabled in this function
//           if using this function, use cli() and sei() before and after
u08 dallasWriteBit(u08 bit);

// dallasReadByte()
//     reads a byte from the 1-wire bus and returns this byte
//     note: global interupts are disabled in this function
//#define dallasReadByte()	(dallasWriteByte(0xFF))
u08 dallasReadByte(void);

void dallasBufferEnabled(u08 isEnabled);
void dallasBufferRead(u08 size);

// dallasWriteByte()
//     writes the passed in byte to the 1-wire bus
//     note: global interupts are disabled in this function.
u08 dallasWriteByte(u08 byte);

// dallasReadRAM()
//     reads the RAM from the specified device, at the specified RAM address
//     for the specified length.  Data is stored into data variable
u08  dallasReadRAM(dallas_rom_id_T* rom_id, u16 addr, u08 len, u08 *data);

// dallasWriteRAM()
//     writes the specified data for the specified length to the RAM
//     located at the specified address of the specified device
u08  dallasWriteRAM(dallas_rom_id_T* rom_id, u16 address, u08 len, u08* data);

// dallasWaitUntilDone()
//     waits until the conversion of a dallas device is done
void dallasWaitUntilDone(void);

// dallasReadROM()
//     finds the ROM code of a device if only 1 device is
//     connected to the bus the ROM value is passed by referenced
//     returns any error that occured or DALLAS_NO_ERROR
u08 dallasReadROM(dallas_rom_id_T* rom_id);

// dallasMatchROM()
//     performs a reset on the 1-wire bus and then
//     selects the specified dallas device on the network
//     returns any error that occured or DALLAS_NO_ERROR
u08  dallasMatchROM(dallas_rom_id_T* rom_id);
#define dallasSkipROM() dallasMatchROM(0)

// dallasCommand
//     performs match rom and send command byte
u08 dallasCommand(dallas_rom_id_T* rom_id, u08 command);

// dallasPrintROM
//     prints the ROM from MSB to LSB in the format: xx xx xx xx xx xx xx xx
void dallasPrintROM(dallas_rom_id_T* rom_id);

// dallasAddressCheck()
//     checks to make sure that the rom id is in the proper family,
//     and if the crc of the id is correct
//     returns the corresponding error or DALLAS_NO_ERROR
u08  dallasAddressCheck(dallas_rom_id_T* rom_id, u08 family);

// dallasFindDevices()
//     finds all the devices on the network, or up to the *count
//     stores the ids in the given array
//     returns any error that occured or DALLAS_NO_ERROR
u08 dallasFindDevices(dallas_rom_id_T rom_id[], u08 *count);

// dallasFindInit()
//     prepares internal variables for device searching
void dallasFindInit(void);

// dallasFindNextDevice()
//     finds devices one by one
//     stores the id in the rom_id
//     stores error in the error
//     returns 0 if device not found
//     function dallasFindInit() must be called before this function called
int dallasFindNextDevice(dallas_rom_id_T *rom_id, u08 *error);

// dallasGetErrorText()
//     returns error text for given error code
char *dallasGetErrorText(u08 error);

#ifdef	__cplusplus
};
#endif

#endif
