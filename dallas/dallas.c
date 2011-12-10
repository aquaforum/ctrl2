// Source for AVR developed by rwatson: http://www.koders.com/c/fid2FCCAB7DEEA488055C7E6F0B9A22960FF71D7C65.aspx
// Ported to ARM by cherep22

//----- Include Files ---------------------------------------------------------
#if defined(_WINDOWS_NT_) || defined(_WINDOWS_CE_)
#include <windows.h>
#endif

#if defined(_LINUX_) || defined(_LINUX_EMBEDDED_)
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <termios.h>
#include <fcntl.h>
#endif


#include <string.h>
#include <stdlib.h>
#include "delay.h"
#include "dallas.h"
#include "crc.h"
#include <stdio.h>

//----- Defines ----------------------------------------------------------------

#define cli()
#define sei()

//----- Global Variables -------------------------------------------------------
static u08 last_discrep = 0;    // last discrepancy for FindDevices
static u08 done_flag = 0;        // done flag for FindDevices

static u08 dallas_crc;                    // current crc global variable

#define dallasCRC(i) crc8_update(dallas_crc, (i))

//----- Functions --------------------------------------------------------------

#ifdef    __cplusplus
extern "C" {
#endif

/*--------------------------------------------------------------------------
 * dallasFindNextDeviceStatic: find the next device on the bus
 * input...................... rom_id - pointer to store the rom id found
 * returns.................... true or false if a device was found
 *-------------------------------------------------------------------------*/
static u08 dallasFindNextDeviceStatic(dallas_rom_id_T* rom_id);


#define dallasDelayUs    delay_us

//------------------------------------------------------
//----------- Platform specific code        ------------

#if defined(_LINUX_) || defined(_LINUX_EMBEDDED_)

#define CHECK_TRUE(f, s) if (!(f)) { snprintf(last_system_error_text, sizeof(last_system_error_text), s, (last_system_error = errno)); return DALLAS_OS_ERROR; }

#define DWORD unsigned int
#define BOOL unsigned int
#define TRUE ((unsigned int)-1)
#define FALSE 0

#define DALLAS_BAUD_RATE_RESET B9600
#define DALLAS_BAUD_RATE_IO    B115200

static int fd;
struct termios options;

int last_system_error;
char last_system_error_text[255];
char formatted_last_system_error_text[512];

char *dallasGetLastSystemErrorText()
{
    size_t len;
    strncpy(formatted_last_system_error_text, last_system_error_text, sizeof formatted_last_system_error_text);
    len = strlen(formatted_last_system_error_text);
    strncpy(formatted_last_system_error_text + len, strerror(last_system_error), sizeof formatted_last_system_error_text - len); 
    return formatted_last_system_error_text;
}

static BOOL dallasSetBaudRate(speed_t dwBaudRate)
{
    int result = 0;
    if (cfgetispeed(&options) != dwBaudRate) {
        cfsetispeed(&options, dwBaudRate);          // Set Baud Rate
        cfsetospeed(&options, dwBaudRate);  	
        result = tcsetattr(fd, TCSANOW, &options);  // Save The Configure
        last_system_error = errno;
        tcflush(fd, TCIOFLUSH);                     // Flush the input (read) buffer
    }
    return !result;
}

static BOOL dallasReadData(u08 * buffer, u08 buffer_size, u08 * actual_size)
{
    DWORD dwBytesRead;
    BOOL result = TRUE;
    dwBytesRead = read(fd, buffer, buffer_size);
    if (dwBytesRead == (DWORD)-1) {
        dwBytesRead = 0;
        last_system_error = errno;
        result = FALSE;
    }
    if (actual_size)
        *actual_size = (u08) dwBytesRead;
    return result;
}

static BOOL dallasWriteData(u08 * buffer, u08 buffer_size)
{
    DWORD dwBytesWritten;
    dwBytesWritten = write(fd, buffer, buffer_size);
    if (dwBytesWritten == (DWORD)-1) {
        last_system_error = errno;
        return FALSE;
    }
    return TRUE;
}

u08 dallasInit(char *PortName)
{
    fd = open(PortName, O_RDWR | O_NOCTTY | O_NDELAY);
    CHECK_TRUE(
        fd != -1,
        "Cannot open COM-port. System error code: 0x%08x\n");

    fcntl(fd, F_SETFL, 0);
    
    CHECK_TRUE(
        tcgetattr(fd, &options) != -1, 
        "Cannot get comm state. System error code: 0x%08x\n");

//    close(fd);
//    CHECK_TRUE(0, "Test error 0: 0x%08x\n");
    
    cfsetispeed(&options, B9600); // Set Baud Rate
    cfsetospeed(&options, B9600);
    
    options.c_cflag &= ~(PARENB | CSIZE | CSTOPB);
    options.c_cflag |= CS8 | CLOCAL | CREAD;
    options.c_iflag &= ~(IXON | IXOFF | IXANY);
    options.c_iflag |=  IGNBRK;
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    options.c_oflag &= ~OPOST;
    options.c_cc[VMIN] = 0;
    options.c_cc[VTIME] = 1;

    CHECK_TRUE(
        tcsetattr(fd, TCSANOW, &options) != -1, 
        "Cannot set comm state. System error code: 0x%08x\n");
        
    tcflush(fd, TCIOFLUSH);
    return DALLAS_NO_ERROR;
}

void dallasDeinit()
{
    close(fd);
}

#endif // defined(_LINUX_) || defined(_LINUX_EMBEDDED_)


#if defined(_WINDOWS_NT_) || defined(_WINDOWS_CE_)

#define DALLAS_BAUD_RATE_RESET CBR_9600
#define DALLAS_BAUD_RATE_IO    CBR_115200

//#define CHECK_TRUE(f, s) if (!(f)) { rprintf((s), GetLastError()); getchar(); return DALLAS_OS_ERROR; }
#define CHECK_TRUE(f, s) if (!(f)) { _snprintf_s(last_system_error_text, sizeof(last_system_error_text), _TRUNCATE, s, GetLastError()); return DALLAS_OS_ERROR; }

static HANDLE hCom;
static DCB dcb;

char last_system_error_text[255];

char *dallasGetLastSystemErrorText()
{
    size_t i;
    size_t len = strlen(last_system_error_text);
    wchar_t w_last_system_error_text[255];
    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_ARGUMENT_ARRAY, 
        0, GetLastError(), 0, w_last_system_error_text, sizeof(w_last_system_error_text), 0);
    wcstombs_s(&i, &last_system_error_text[len], sizeof(last_system_error_text) - len, w_last_system_error_text, _TRUNCATE);
    return last_system_error_text;
//  // LINUX
//  Result := strerror_r(ErrorCode, Buffer, sizeof(Buffer));
}

static BOOL dallasSetBaudRate(DWORD dwBaudRate)
{
    if (dcb.BaudRate != dwBaudRate) {
        dcb.BaudRate = dwBaudRate;
        return SetCommState(hCom, &dcb);
    }
    return TRUE;
}

static BOOL dallasReadData(u08 * buffer, u08 buffer_size, u08 * actual_size)
{
    DWORD dwBytesRead;
    BOOL result;
    result = ReadFile(hCom, buffer, buffer_size, &dwBytesRead, NULL);
    if (actual_size)
        *actual_size = (u08) dwBytesRead;
    return result;
}

static BOOL dallasWriteData(u08 * buffer, u08 buffer_size)
{
    DWORD dwBytesWritten;
    return WriteFile(hCom, buffer, buffer_size, &dwBytesWritten, NULL);
}


/*
if ( wpurl )
{
  int len = strlen(wpurl)+1;
  wchar_t *wText = new wchar_t[len];
  if ( wText == 0 )
    return;
  memset(wText,0,len);
  ::MultiByteToWideChar(  CP_ACP, NULL,wpurl, -1, wText,len );

  // when finish using wText dont forget to delete it
  delete []wText;

}*/

#define MAX_PORT_NAME_LENGTH 256

u08 dallasInit(char *PortName)
{
    COMMTIMEOUTS cto;

    wchar_t wPortName[MAX_PORT_NAME_LENGTH];
    memset(wPortName, 0, sizeof(wPortName));
    MultiByteToWideChar(CP_ACP, 0, PortName, -1, wPortName, sizeof(wPortName));

    hCom = CreateFile(wPortName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0 /*FILE_FLAG_OVERLAPPED*/, NULL);
    CHECK_TRUE(
        hCom != INVALID_HANDLE_VALUE,
        "Cannot open COM-port. System error code: 0x%08x\n");

    CHECK_TRUE(
        GetCommState(hCom, &dcb), 
        "Cannot get comm state. System error code: 0x%08x\n");
    memset(&dcb, 0, sizeof(dcb));
    dcb.DCBlength = sizeof(dcb);
    dcb.ByteSize = 8;
    dcb.BaudRate = CBR_110;
    //BuildCommDCB(L"baud=1200 parity=N data=8 stop=1", &dcb);
    //BuildCommDCB(L"baud=9600 parity=N data=8 stop=1 to=off xon=off odsr=off octs=off dtr=off rts=off idsr=off", &dcb),
    //CHECK_TRUE(
    //    BuildCommDCB(L"baud=9600 parity=N data=8 stop=1 to=off xon=off odsr=off octs=off dtr=off rts=off idsr=off", &dcb),
    //    "Cannot build dcb. System error code: 0x%08x\n");

    //dcb.StopBits = ONESTOPBIT; // TWOSTOPBITS;
    //dcb.StopBits = TWOSTOPBITS;
//    dcb.fParity = TRUE;
//    dcb.Parity = MARKPARITY;
    dcb.BaudRate = CBR_110;
//    dcb.StopBits = TWOSTOPBITS;

    CHECK_TRUE(
        GetCommTimeouts(hCom, &cto),
        "Cannot get port timeouts. System error code: 0x%08x\n");

    cto.ReadIntervalTimeout = 100;
    cto.ReadTotalTimeoutConstant = 100;
    cto.ReadTotalTimeoutMultiplier = 100;
    cto.WriteTotalTimeoutConstant = 0;
    cto.WriteTotalTimeoutMultiplier = 0;

    CHECK_TRUE(
        SetCommTimeouts(hCom, &cto),
        "Cannot set port timeouts. System error code: 0x%08x\n");

    return DALLAS_NO_ERROR;
}

void dallasDeinit()
{
    CloseHandle(hCom);
}

#endif // defined(_WINDOWS_NT_) || defined(_WINDOWS_CE_)

//----------- End of platform specific code ------------
//------------------------------------------------------


u08 dallasReset(void)
{
    unsigned char c, i;

    CHECK_TRUE(
        dallasSetBaudRate(DALLAS_BAUD_RATE_RESET) != FALSE,
        "Cannot set baud rate. System error code: 0x%08x\n");

    c = 0xF0;
    for (i=0; c == 0xF0 && i < 3; ++i) {
    CHECK_TRUE(
        dallasWriteData(&c, 1) != FALSE, 
        "Cannot write data to port. System error code: 0x%08x\n");

    CHECK_TRUE(
        dallasReadData(&c, 1, NULL) != FALSE,
        "Cannot read data from port. System error code: 0x%08x\n");
    }

    if (c == 0xF0)
        return DALLAS_NO_PRESENCE;

    return DALLAS_NO_ERROR;
}

u08 dallasWriteBit(u08 bit)
{
    unsigned char c;

    CHECK_TRUE(
        dallasSetBaudRate(DALLAS_BAUD_RATE_IO),
        "Cannot set baud rate. System error code: 0x%08x\n");

    c = bit ? 0xFF : 0;

    CHECK_TRUE(
        dallasWriteData(&c, 1), 
        "Cannot write data to port. System error code: 0x%08x\n");

    CHECK_TRUE(
        dallasReadData(&c, 1, NULL), 
        "Cannot read data from port. System error code: 0x%08x\n");

    return c == 0xFF ? 1 : 0;
}

#define DALLAS_MAX_RETRY_COUNT         	6
#define DALLAS_BUFFER_SIZE            128

static unsigned char dallas_buffer[DALLAS_BUFFER_SIZE];
static u08 dallas_buffer_size, dallas_buffer_in_size, dallas_buffer_in_pos, dallas_buffer_out_pos, dallas_buffer_enabled;

u08 dallasWriteBits(u08 * pByte, u08 bit_count)
{
    u08 i, j;
    u08 *pb;
    unsigned char buffer[DALLAS_BUFFER_SIZE * 8];
    u08 uBytesRead;

    if (bit_count <= 0)
        return 0;

    CHECK_TRUE(
        dallasSetBaudRate(DALLAS_BAUD_RATE_IO),
        "Cannot set baud rate. System error code: 0x%08x\n");

    pb = pByte;
    for (i=0;i<bit_count;i++) {
        buffer[i] = (*pb & 1 ? 0xFF : 0);
        *pb >>= 1;
        if ((i & 7) == 7)
            pb++;
    }

    CHECK_TRUE(
        dallasWriteData(&buffer[0], bit_count), 
        "Cannot write data to port. System error code: 0x%08x\n");

    for (i=0,j=0; i<bit_count; i += uBytesRead) {
        CHECK_TRUE(
            dallasReadData(&buffer[i], bit_count - i, &uBytesRead),
            "Cannot read data from port. System error code: 0x%08x\n");
        if (uBytesRead > 0)
            j = 0;
        else if (++j == DALLAS_MAX_RETRY_COUNT)
            break;        
    }
    uBytesRead = i;

    if (uBytesRead != bit_count) {
        // rprintf("bitCount = %d, uBytesRead = %d\n", bit_count, uBytesRead);
        return 0;
    }

    pb = pByte - 1;
    for (i=0;i<bit_count;i++) {
        if ((i & 7) == 0)
            *++pb = 0;
        *pb >>= 1;
        *pb |= (buffer[i] == 0xFF ? 0x80 : 0);
    }

    if (bit_count & 7)
        *pb >>= 8 - (bit_count & 7);

    return 0;
}

u08 dallasWriteByteUnbuffered(u08 byte, u08 bit_count)
{
    dallasWriteBits(&byte, bit_count);
    return byte;
}

void dallasFlushBuffer()
{
    if (dallas_buffer_out_pos < dallas_buffer_size)
        dallasWriteBits(&dallas_buffer[dallas_buffer_out_pos], (dallas_buffer_size - dallas_buffer_out_pos) * 8);
    if (!dallas_buffer_in_size)
        dallas_buffer_size = 0;
    dallas_buffer_out_pos = dallas_buffer_size;
}

void dallasBufferEnabled(u08 isEnabled)
{
#ifdef _DALLAS_BUFFER_ENABLED_
    if (dallas_buffer_enabled != isEnabled && dallas_buffer_enabled) {
        dallasFlushBuffer();
        dallas_buffer_size = 0;
        dallas_buffer_out_pos = 0;
        dallas_buffer_in_pos = 0;
        dallas_buffer_in_size = 0;
    }
    dallas_buffer_enabled = isEnabled;
#endif
}

u08 dallasWriteByte(u08 byte)
{
    if (dallas_buffer_enabled && dallas_buffer_size < DALLAS_BUFFER_SIZE) {
        dallas_buffer[dallas_buffer_size++] = byte;
        if (dallas_buffer_size == DALLAS_BUFFER_SIZE)
            dallasFlushBuffer();
    }
    else
        dallasWriteBits(&byte, 8);
    return byte;
}

void dallasBufferRead(u08 size)
{
    u08 i;
    if (!dallas_buffer_enabled)
        return;
    if (size > DALLAS_BUFFER_SIZE)
        size = DALLAS_BUFFER_SIZE;
    dallas_buffer_in_size = 0;
    if (dallas_buffer_size + size > DALLAS_BUFFER_SIZE)
        dallasFlushBuffer();
    dallas_buffer_in_pos = dallas_buffer_size;
    dallas_buffer_in_size = size;
    for (i = 0; i < size; ++i)
        dallasWriteByte(0xFF);
    dallasFlushBuffer();
}

u08 dallasReadByte()
{
    u08 b = (dallas_buffer_in_pos < dallas_buffer_size) 
        ? dallas_buffer[dallas_buffer_in_pos++] 
        : dallasWriteByte(0xFF);
    if (dallas_buffer_in_pos > 0 && dallas_buffer_in_pos == dallas_buffer_size) {
        dallas_buffer_size = 0;
        dallas_buffer_out_pos = 0;
        dallas_buffer_in_pos = 0;
        dallas_buffer_in_size = 0;
    }
    return b;
}

u08 dallasReadSelectedRAM(u16 addr, u08 len, u08 *data)
{
    u08 i, j;
    u16 crc = 0;

    // first make sure we actually have something to do
    if (data == NULL)
        return DALLAS_NULL_POINTER;
    if (len == 0)
        return DALLAS_ZERO_LEN;

    // enter read mode
    dallasWriteByte(DALLAS_READ_MEMORY);
    crc = crc16_update(crc, DALLAS_READ_MEMORY);
    
    // write address one byte at a time
    dallasWriteByte(addr & 0x00FF);
    dallasWriteByte(addr >> 8);
    crc = crc16_update(crc, addr & 0x00FF);
    crc = crc16_update(crc, addr >> 8);
    
    dallasBufferRead(10 - (addr & 7));

    // read data from device 1 byte at a time
    for (i = 0, j = addr & 7; i < len; i++)
    {
        data[i] = dallasReadByte();
        crc = crc16_update(crc, data[i]);
        j++;
        j &= 7;
        if (!j)
        {
	//		wchar_t buf[255];
	//		u08 b1 = dallasReadByte(), b2 = dallasReadByte();

	//		wsprintf(buf, L"%02x  %02x %02x  %02x %02x %02x %02x %02x %02x %02x %02x  %02x %02x  %04x", 
	//			DALLAS_READ_MEMORY, addr & 0x00FF, addr >> 8, 
	//			data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], 
	//			b1, b2, crc);
	//		MessageBox(0, buf, L"CRC error", MB_OK);
            crc = ~crc;
            crc ^= dallasReadByte();
            crc ^= dallasReadByte() << 8;
			//crc ^= b1;
            //crc ^= b2 << 8;
            if (crc) {
                dallasBufferEnabled(0);
                return DALLAS_CRC_ERROR;
            }
            dallasBufferRead((len - i - 1 < 8) ? (len - i - 1) : 10);
        }
    }

    return DALLAS_NO_ERROR;
}

u08 dallasReadRAM(dallas_rom_id_T* rom_id, u16 addr, u08 len, u08 *data)
{
    u08 result;

    // first make sure we actually have something to do
    if (data == NULL)
        return DALLAS_NULL_POINTER;
    if (len == 0)
        return DALLAS_ZERO_LEN;

    dallasBufferEnabled(1);

    // reset the bus and request the device
    DALLAS_CHECK(dallasMatchROM(rom_id));

    result = dallasReadSelectedRAM(addr, len, data);

    dallasBufferEnabled(0);

    return result;
}

u08 dallasWriteRAM(dallas_rom_id_T* rom_id, u16 addr, u08 len, u08* data)
{
    u08 i;
    u16 crc = 0;

    // first make sure we actually have something to do
    if (data == NULL)
        return DALLAS_NULL_POINTER;
    if (len == 0)
        return DALLAS_ZERO_LEN;

    dallasBufferEnabled(1);

    // reset the bus and request the device
    DALLAS_CHECK(dallasMatchROM(rom_id));
    
    // enter write mode
    dallasWriteByte(DALLAS_WRITE_MEMORY);
    crc = crc16_update(crc, DALLAS_WRITE_MEMORY);

    // write address one byte at a time
    dallasWriteByte(addr & 0x00FF);
    dallasWriteByte(addr >> 8);
    crc = crc16_update(crc, addr & 0x00FF);
    crc = crc16_update(crc, addr >> 8);

    // write data one byte at a time
    for(i=0;i<len;crc = addr + ++i)
    {
        dallasWriteByte(data[i]);
        crc = crc16_update(crc, data[i]);
        
        dallasBufferRead(3);

        // verify crc
        crc = ~crc;
        crc ^= dallasReadByte();
        crc ^= dallasReadByte() << 8;
        if (crc)
        {
            dallasBufferEnabled(0);
            return DALLAS_CRC_ERROR;
        }

        // verify the data
        if (dallasReadByte() != data[i]) {
            dallasBufferEnabled(0);
            return DALLAS_VERIFY_ERROR;
        }
    }

    dallasBufferEnabled(0);
    return DALLAS_NO_ERROR;
}

void dallasWaitUntilDone(void)
{
    // wait until we recieve a one
    //while(!dallasReadBit());

    delay_ms(6);	// for ds2450: channels * bits * 80 us + 180 us = 4 * 16 * 80 + 160 = 5280 us
}

u08 dallasReadROM(dallas_rom_id_T* rom_id)
{
    u08 i;

    // reset the 1-wire bus and look for presence
    DALLAS_CHECK(dallasReset());
    
    // send READ ROM command
    dallasWriteByte(DALLAS_READ_ROM);

    // get the device's ID 1 byte at a time
    for(i=0;i<8;i++)
        rom_id->byte[i] = dallasReadByte();

    return DALLAS_NO_ERROR;
}

u08 dallasMatchROM(dallas_rom_id_T* rom_id)
{
    u08 i;

    // reset the 1-wire and look for presence
    DALLAS_CHECK(dallasReset());

    if (rom_id)
    {
        // send MATCH ROM command
        dallasWriteByte(DALLAS_MATCH_ROM);
    
        // write id one byte at a time
        for(i=0;i<8;i++)
            dallasWriteByte(rom_id->byte[i]);
    }
    else
        // send SKIP ROM command
        dallasWriteByte(DALLAS_SKIP_ROM);

    return DALLAS_NO_ERROR;
}


u08 dallasCommand(dallas_rom_id_T* rom_id, u08 command)
{
    DALLAS_CHECK(dallasMatchROM(rom_id));
    dallasWriteByte(command);
    return DALLAS_NO_ERROR;
}

//void dallasPrintROM(dallas_rom_id_T* rom_id)
//{
//    // print out the rom in the format: xx-xxxxxxxxxxxx-xx
//    rprintf("%02x-%02x%02x%02x%02x%02x%02x-%02x\r\n",
//        rom_id->byte[7], rom_id->byte[6], rom_id->byte[5], rom_id->byte[4], 
//        rom_id->byte[3], rom_id->byte[2], rom_id->byte[1], rom_id->byte[0]);
//}

u08 dallasAddressCheck(dallas_rom_id_T* rom_id, u08 family)
{
    u08 i;

    if (rom_id) {
        dallas_crc = 0;
        for(i=0;i<8;i++)
            dallasCRC(rom_id->byte[i]);

        if (dallas_crc || rom_id->byte[DALLAS_FAMILY_IDX] != family)
            return DALLAS_ADDRESS_ERROR;
    }

    return DALLAS_NO_ERROR;
}

u08 dallasFindDevices(dallas_rom_id_T rom_id[], u08 *count)
{
    u08 error, num_found = 0;
    dallas_rom_id_T id;

    dallasFindInit();
    while (num_found < *count && dallasFindNextDevice(&id, &error))
        memcpy(&rom_id[num_found++], &id, 8);

    *count = num_found;
    return error;
}

void dallasFindInit()
{
    // reset the rom search last discrepancy global
    last_discrep = 0;
    done_flag = FALSE;
}

int dallasFindNextDevice(dallas_rom_id_T *rom_id, u08 *error)
{
    u08 err;
    u08 is_first_device = (last_discrep == 0);

    if (done_flag) {
        if (error)
            *error = DALLAS_NO_ERROR;
        return 0;
    }

    err = dallasFindNextDeviceStatic(rom_id);
    if (error)
        *error = (is_first_device && err == DALLAS_NO_PRESENCE) ? DALLAS_NO_ERROR : err;  // bus can be empty, it is not error for caller

    return err == DALLAS_NO_ERROR;
}

static u08 dallasFindNextDeviceStatic(dallas_rom_id_T *rom_id)
{
    u08 bit;
    u08 i = 0;
    u08 bit_index = 1;
    u08 byte_index = 0;
    u08 bit_mask = 1;
    u08 discrep_marker = 0;
    u08 two_bits = 0x80;
    
    // reset the CRC
    dallas_crc = 0;

    DALLAS_CHECK(dallasReset());

    // send search ROM command
    dallasWriteByte(DALLAS_SEARCH_ROM);

    // loop until through all 8 ROM bytes
    while(byte_index<8)
    {
        // read line 2 times to determine status of devices
        //    00 - devices connected to bus with conflicting bits
        //    01 - all devices have a 0 in this position
        //    10 - all devices ahve a 1 in this position
        //    11 - there are no devices connected to bus
        if (two_bits & 0x80) {
            two_bits = dallasWriteByteUnbuffered(0xFF, 2);
        }
        i = 0;
        if (two_bits & 1)
            i = 2;                // store the msb if 1
        if (two_bits & 2)
            i |= 1;                // store the lsb if 1
        
        if (i==3)
        {
            // there are no devices on the 1-wire with selected address
            return DALLAS_DEVICE_ERROR;
        }
        else
        {
            if (i>0)
            {
                // all devices coupled have 0 or 1
                // shift 1 to determine if the msb is 0 or 1
                bit = i>>1;
            }
            else
            {
                // if this discrepancy is before the last discrepancy on a
                // previous FindNextDevice then pick the same as last time
                if (bit_index<last_discrep)
                    bit = ((rom_id->byte[byte_index] & bit_mask) > 0);
                else
                    bit = (bit_index==last_discrep);
                
                // if 0 was picked then record position with bit mask
                if (!bit)
                    discrep_marker = bit_index;
            }

            // isolate bit in rom_id->byte[byte_index] with bit mask
            if (bit)
                rom_id->byte[byte_index] |= bit_mask;
            else
                rom_id->byte[byte_index] &= ~bit_mask;

            // ncrement bit index counter and shift the bit mask
            bit_index++; 
            bit_mask <<= 1;
            
            if (!bit_mask)
            {
                // if the mask is 0 then go to new ROM
                // accumulate the CRC and incriment the byte index and bit mask
                dallasCRC(rom_id->byte[byte_index]);
                byte_index++;
                bit_mask++;
            }
            // ROM search write
            if (byte_index < 8)
                two_bits = dallasWriteByteUnbuffered(bit | 6, 3) >> 1;
            else
                dallasWriteBit(bit);
        }
    }

    if (dallas_crc)
    {
        // search was unsuccessful - reset the last discrepancy to 0 and return false
        last_discrep = 0;
        return DALLAS_CRC_ERROR;
    }

    // search was successful, so set last_discrep and done_flag
    last_discrep = discrep_marker;
    done_flag = (last_discrep==0);

    return DALLAS_NO_ERROR;
}

char *dallasGetErrorText(u08 error)
{
    switch (error)
    {
        case DALLAS_NO_ERROR:
            return "No errors";
        case DALLAS_NO_PRESENCE:
            return "No presence detected or connection is lost";
        case DALLAS_INVALID_CHANNEL:
            return "Invalid channel";
        case DALLAS_VERIFY_ERROR:
            return "Data verification error";
        case DALLAS_ADDRESS_ERROR:
            return "Bad address";
        case DALLAS_CRC_ERROR:
            return "Invalid data CRC";    
        case DALLAS_DEVICE_ERROR:
            return "No response or connection is lost";
        case DALLAS_FORMAT_ERROR:
            return "Bad return format";
        case DALLAS_NULL_POINTER:
            return "Null Pointer";
        case DALLAS_ZERO_LEN:
            return "RAM rd/wr 0 bytes";
        case DALLAS_BUS_ERROR:
            return "Bus error, check pullup";
        case DALLAS_RESOLUTION_ERROR:
            return "resolution out of range";
        case DALLAS_OS_ERROR:
            return dallasGetLastSystemErrorText();
        default:
            return "Unknown";
    }
}

#ifdef    __cplusplus
};
#endif

