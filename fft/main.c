/*-----------------------------------------------------------------------------
/
/
/----------------------------------------------------------------------------*/
#include <avr/io.h>	
#include "xprintf.h"
#include <util/delay.h>
#include "xmega_digital.h"
#include "ffft.h"

int16_t getch();
void init_uart();
void sendch(uint8_t ch);
void initClock_32Mhz();

/* Synthetic data */
int16_t capture[FFT_N] = {
	0,12539,23170,30273,32767,30273,23170,12539,0,-12540,-23171,-30274,-32768,-30274,-23171,-12540,
	0,12539,23170,30273,32767,30273,23170,12539,0,-12540,-23171,-30274,-32768,-30274,-23171,-12540,
	0,12539,23170,30273,32767,30273,23170,12539,0,-12540,-23171,-30274,-32768,-30274,-23171,-12540,
	0,12539,23170,30273,32767,30273,23170,12539,0,-12540,-23171,-30274,-32768,-30274,-23171,-12540,
	0,12539,23170,30273,32767,30273,23170,12539,0,-12540,-23171,-30274,-32768,-30274,-23171,-12540,
	0,12539,23170,30273,32767,30273,23170,12539,0,-12540,-23171,-30274,-32768,-30274,-23171,-12540,
	0,12539,23170,30273,32767,30273,23170,12539,0,-12540,-23171,-30274,-32768,-30274,-23171,-12540,
	0,12539,23170,30273,32767,30273,23170,12539,0,-12540,-23171,-30274,-32768,-30274,-23171,-12540,
	0,12539,23170,30273,32767,30273,23170,12539,0,-12540,-23171,-30274,-32768,-30274,-23171,-12540,
	0,12539,23170,30273,32767,30273,23170,12539,0,-12540,-23171,-30274,-32768,-30274,-23171,-12540,
	0,12539,23170,30273,32767,30273,23170,12539,0,-12540,-23171,-30274,-32768,-30274,-23171,-12540,
	0,12539,23170,30273,32767,30273,23170,12539,0,-12540,-23171,-30274,-32768,-30274,-23171,-12540,
	0,12539,23170,30273,32767,30273,23170,12539,0,-12540,-23171,-30274,-32768,-30274,-23171,-12540,
	0,12539,23170,30273,32767,30273,23170,12539,0,-12540,-23171,-30274,-32768,-30274,-23171,-12540,
	0,12539,23170,30273,32767,30273,23170,12539,0,-12540,-23171,-30274,-32768,-30274,-23171,-12540,
	0,12539,23170,30273,32767,30273,23170,12539,0,-12540,-23171,-30274,-32768,-30274,-23171,-12540
};

complex_t bfly_buff[FFT_N];		/* FFT buffer */
uint16_t spektrum[FFT_N/2];		/* Spectrum output buffer */

int main()
{
	int i = 0;
	initClock_32Mhz();
	init_uart();

	/* Onboard LED */
	pinMode(C,7,OUTPUT);	
	while(1)
	{		
		/* Computing the spektrum takes around 8ms with 32Mhz Clock */
		digitalWrite(C,7,HIGH);		
			fft_input(capture, bfly_buff);				
			fft_execute(bfly_buff);		
			fft_output(bfly_buff, spektrum);	
		digitalWrite(C,7,LOW);		
		xprintf("\r\n");
		xprintf("-----------------------------------------------------\r\n");
		for(i=0;i<FFT_N/2;i++)
		{
			xprintf("[%3d]: %5u  |   ",i,spektrum[i]);
			if(((i+1)%8) == 0)
			{
				xprintf("\r\n");
			}
		}
		xprintf("\r\n");
		_delay_ms(1000);
	}
}

void sendch(uint8_t ch)
{
	while(!(USARTD0.STATUS & USART_DREIF_bm));
	USARTD0.DATA = ch;
}

void init_uart()
{
	/* Set UART pin driver states */
	pinMode(D,6,INPUT);
	pinMode(D,7,OUTPUT);

	/* Remap the UART pins */
	PORTD.REMAP = PORT_USART0_bm;

	/* 9600 baud rate UART with 32 Mhz clock */
	USARTD0.CTRLB = USART_RXEN_bm|USART_TXEN_bm;
	USARTD0.CTRLC = USART_CMODE_ASYNCHRONOUS_gc|USART_PMODE_DISABLED_gc|USART_CHSIZE_8BIT_gc;
	USARTD0.BAUDCTRLA = 0xF5; 
	USARTD0.BAUDCTRLB = (0XC<<USART_BSCALE_gp)|0X0C; 

	/* Connect the xprintf library to UART hardware */
	xdev_out(sendch);
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
