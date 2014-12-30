/*-----------------------------------------------------------------------------
/
/
/----------------------------------------------------------------------------*/
#include <avr/io.h>	
#include "xprintf.h"
#include <util/delay.h>
#include "xmega_digital.h"
#include "spiFlash.h"

void init_uart();
void sendch(uint8_t ch);
void initClock_32Mhz();

uint8_t txBuff[256];
uint8_t rxBuff[256];

int main()
{
	int i = 0;
	uint8_t val;
	initClock_32Mhz();
	init_uart();

	/* Wait any key ... */
	while(!(USARTD0.STATUS & USART_RXCIF_bm));

	/* SPI Flash connections */
	pinMode(C,0,OUTPUT); // CS
	pinMode(C,1,INPUT); // SO
	pinMode(C,2,INPUT); // WP
	pinMode(C,3,OUTPUT); // SI
	pinMode(C,4,OUTPUT); // SCK
	pinMode(C,5,INPUT); // HOLD

	xprintf("Status: %d\r\n",spiFlash_check_ID());

	for(i=0;i<256;i++)
	{
		txBuff[i] = i;
	}

	spiFlash_write_enable();
	spiFlash_sectorErase(0x00);
	while(spiFlash_isBusy());
	spiFlash_write_disable();

	spiFlash_write_enable();
	spiFlash_write_data(0,256,txBuff);		
	while(spiFlash_isBusy());
	spiFlash_write_disable();

	xprintf("Before:\r\n");
	put_dump(rxBuff,0,256,DW_CHAR);

	spiFlash_read_data(0,256,rxBuff);

	xprintf("After:\r\n");
	put_dump(rxBuff,0,256,DW_CHAR);

	while(1);
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
