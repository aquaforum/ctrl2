// ************************************************************************************************
///
/// \file 			ds2408.h
/// \brief			header file of ds2408.c
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
                         
#ifndef _DS2408_H_INCLUDED_
#define _DS2408_H_INCLUDED_
                               
              
//DS2408 commands
#define SKIP_ROM			0xCC	// skip rom command
#define READ_PIO			0xF0    // read pio registers command
#define CAW					0x5A	// channel-access-write command
#define WRITE_REGISTER		0xCC	// write conditional search register

//DS2408 target addresses
#define LOGIC_STATE			0x88	// address of PIO Logic State Register
#define OUT_LATCH_STATE		0x89	// address of PIO Output Latch State Register
#define CONTROL_STATUS		0x8D	// address of Control/Status Register
#define ADR_NULL			0x00

#define PLS_PIN				0x00		
#define PLS_LATCH			0x01
#define CT_OR				0x00
#define CT_AND				0x02
#define RSTZ_RST			0x00	// RSTS is _RST_
#define RSTZ_STRB			0x04	// RSTS is _STRB_
#define PORL				0x08	// power on reset latch (Read, only 0 can be written to clear PORL)
#define VCCP				0x80	// VCC Power Status (Read-Only)

#define DS2408_FAMILY		0x29

#include "types.h"
#include "dallas.h"

#ifdef	__cplusplus
extern "C" {
#endif

extern u08	ds2408_write_register(dallas_rom_id_T *id, u08 address, u08 data);
extern u08	ds2408_write_output(dallas_rom_id_T *id, unsigned char data);
extern u08	ds2408_read_output(dallas_rom_id_T *id, u08 *result);
extern u08	ds2408_read_input(dallas_rom_id_T *id, u08 *result);
extern u08	ds2408_write_bits(dallas_rom_id_T *id, unsigned char bitmask, unsigned char value);
extern u08	ds2408_set_bits(dallas_rom_id_T *id, unsigned char bitmask);
extern u08	ds2408_clear_bits(dallas_rom_id_T *id, unsigned char bitmask);

#ifdef	__cplusplus
};
#endif

//#ifdef COMMON_LIBRARY_ENABLE
//#pragma library ds2408.c
//#endif

#endif	// #ifndef _DS2408_H_INCLUDED_
