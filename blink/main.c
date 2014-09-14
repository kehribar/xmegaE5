#include <avr/io.h>	
#include <util/delay.h>
#include "xmega_digital.h"

void initClock_32Mhz();

int main()
{
	initClock_32Mhz();

	pinMode(C,7,OUTPUT);

	while(1)
	{
		_delay_ms(500);
		togglePin(C,7);
	}
}

void initClock_32Mhz()
{
	/* Generates 32Mhz clock from internal 2Mhz clock via PLL */
	OSC.PLLCTRL = OSC_PLLSRC_RC2M_gc | 16;
	OSC.CTRL |= OSC_PLLEN_bm ;
	while((OSC.STATUS & OSC_PLLRDY_bm) == 0);
	CCP = CCP_IOREG_gc;
	CLK.CTRL = CLK_SCLKSEL_PLL_gc;
}
