// ************************************************************************************************
///
/// \file 			ds2408.c
/// \brief			DS2408 device driver
///
///					DS2408 is an 1-wire Port Expander
///
/// \date	 		$LastChangedDate: 2007-03-06 21:48:07 +0100 (Di, 06 Mrz 2007) $
/// \version 		$Rev: 606 $
///	\author  		Carsten Kögler, Thomas Kellner<br> Copyright (c) FTZ Leipzig <br> 
///					D-04107 Leipzig<br>	Wächterstr. 13 info@easytoweb.de
/// \par License    
///					This library is free software; you can redistribute it and/or modify it 
///					under the terms of the GNU Lesser General Public License as published by 
///					the Free Software Foundation; either version 2.1 of the License, or 
///					(at your option) any later version. This library is distributed in the hope 
///					that it will be useful, but WITHOUT ANY WARRANTY; without even the implied 
///					warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
///					See the GNU Lesser General Public License for more details.
///					see: http://www.gnu.org/copyleft/lesser.html	
///					You should have received a copy of the GNU Lesser General Public License 
///					along with this library; if not, write to the Free Software Foundation, 
///					Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA <br><br>
///					Die Bibliothek ist freie Software; Sie dürfen sie unter den Bedingungen der 
///					GNU Lesser General Public License, wie von der Free Software Foundation 
///					veröffentlicht, weiterverteilen und/oder modifizieren; entweder gemäß 
///					Version 2.1 der Lizenz oder (nach Ihrer Option) jeder späteren Version. 
///					Diese Bibliothek wird in der Hoffnung weiterverbreitet, daß sie nützlich 
///					sein wird, jedoch OHNE IRGENDEINE GARANTIE, auch ohne die implizierte 
///					Garantie der MARKTREIFE oder der VERWENDBARKEIT FÜR EINEN BESTIMMTEN ZWECK. 
///					Mehr Details finden Sie in der GNU Lesser General Public License.
///					see: http://www.gnu.org/copyleft/lesser.de.html	
///					Sie sollten eine Kopie der GNU Lesser General Public License zusammen mit 
///					dieser Bibliothek erhalten haben; falls nicht, schreiben Sie an die FSF,
///					Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
///
// ************************************************************************************************

// Original: https://141.57.27.10/svn/avr-common-source/tags/1.5.0/devices/
// Ported to ARM by cherep22

#define DS2408_ENABLE
#ifdef DS2408_ENABLE

#include "ds2408.h"

#ifdef DS2408_BUFFER
signed int ds2408_buffer=-1;
#endif

#define W1_WRITE dallasWriteByte
#define W1_READ dallasReadByte

#ifdef	__cplusplus
extern "C" {
#endif

u08	ds2408_write_register(dallas_rom_id_T *id, u08 address, u08 data)
{
	dallasBufferEnabled(1);
	DALLAS_CHECK(dallasCommand((dallas_rom_id_T *)&id[0], WRITE_REGISTER));
	W1_WRITE(address);
	W1_WRITE(ADR_NULL);
	W1_WRITE(data);
	dallasBufferEnabled(0);
	return DALLAS_NO_ERROR;
}

// **********************************************************************************
/// \brief  	DS2408 Ausgaenge setzen (Channel Access Write)
/// @param[in]  data 8-Bit data
///
// **********************************************************************************
u08 ds2408_write_output(dallas_rom_id_T *id, unsigned char data)
{
#ifdef DS2408_BUFFER
	if (ds2408_buffer==data)
		return DALLAS_NO_ERROR;
	else
		ds2408_buffer = data;
#endif
	dallasBufferEnabled(1);
	// channel-access-write
	DALLAS_CHECK(dallasCommand((dallas_rom_id_T *)&id[0], CAW));
	W1_WRITE(data);			// write byte to PIO
	W1_WRITE(~data);		// write inverted byte to PIO
	dallasBufferEnabled(0);
	return DALLAS_NO_ERROR;
}


// **********************************************************************************
/// \brief  	Zustand der Ausgaenge des DS2408 einlesen  (Read PIO Register)
/// @return  	8-Bit Data Status
///
// **********************************************************************************
u08 ds2408_read_output(dallas_rom_id_T *id, u08 *result)
{
#ifdef DS2408_BUFFER
	*result = ds2408_buffer;
#else
	dallasBufferEnabled(1);
	DALLAS_CHECK(dallasCommand((dallas_rom_id_T *)&id[0], READ_PIO));
	W1_WRITE(OUT_LATCH_STATE);
	W1_WRITE(ADR_NULL);
	dallasBufferRead(1);
	*result = W1_READ();	// read PIO Output Latch State Register
	dallasBufferEnabled(0);
#endif
	return DALLAS_NO_ERROR;
}


// **********************************************************************************
/// \brief  	DS2408 Eingaenge lesen (Channel Access Read)
/// @return  	8-Bit data
///
// **********************************************************************************
u08 ds2408_read_input(dallas_rom_id_T *id, u08 *result)
{
	dallasBufferEnabled(1);
	DALLAS_CHECK(dallasCommand((dallas_rom_id_T *)&id[0], READ_PIO));
	W1_WRITE(LOGIC_STATE);
	W1_WRITE(ADR_NULL);
	dallasBufferRead(1);
	*result = W1_READ();
	dallasBufferEnabled(0);
	return DALLAS_NO_ERROR;
}


// **********************************************************************************
/// \brief  	Mehrere Bit am DS2408 auf vorgegebenen Wert schreiben (setzen+löschen)
/// @param[in] 	bitmask - Bitmaske für die Bits, die beeinflusst werden sollen
/// @param[in] 	value - neuer Wert für die Bits -> bitmask
// **********************************************************************************
u08 ds2408_write_bits(dallas_rom_id_T *id, unsigned char bitmask, unsigned char value)
{
	unsigned char pattern;
	DALLAS_CHECK(ds2408_read_output(id, &pattern));
	pattern &= ~bitmask;		// alten Wert löschen
	pattern |= value;			// neuen Wert einspielen
	return ds2408_write_output(id, pattern);
}


// **********************************************************************************
/// \brief  	Mehrere Bit am DS2408 setzen
/// @param[in] 	bitmask - Bitmaske für die Bits, die gesetzt werden sollen
///
// **********************************************************************************
u08 ds2408_set_bits(dallas_rom_id_T *id, unsigned char bitmask)
{
	unsigned char pattern;
	DALLAS_CHECK(ds2408_read_output(id, &pattern));
	pattern |= bitmask;
	return ds2408_write_output(id, pattern);
}


// **********************************************************************************
/// \brief  	Mehrere Bit am DS2408 löschen
/// @param[in] 	bitmask - Bitmaske für die Bits, die gelöscht werden sollen
///
// **********************************************************************************
u08 ds2408_clear_bits(dallas_rom_id_T *id, unsigned char bitmask)
{
	unsigned char pattern;
	DALLAS_CHECK(ds2408_read_output(id, &pattern));
	pattern &= ~bitmask;
	return ds2408_write_output(id, pattern);
}

#ifdef	__cplusplus
};
#endif

#endif	// #ifdef DS2408_ENABLE
