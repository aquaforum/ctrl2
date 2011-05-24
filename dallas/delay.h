#ifndef _delay_h_
#define _delay_h_

#include "types.h"

#ifdef	__cplusplus
extern "C" {
#endif

#define delay_us(delayval) delay_us_(delayval)

void delay_us_(u32 delayval);
void delay_ms(u32 delayval);

#ifdef	__cplusplus
};
#endif

#endif
