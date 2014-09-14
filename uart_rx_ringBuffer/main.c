/*-----------------------------------------------------------------------------
/
/
/----------------------------------------------------------------------------*/
#include <avr/io.h>	
#include <util/delay.h>
#include "xmega_digital.h"
#include <avr/interrupt.h>
#include "LightweightRingBuff.h"

int16_t getch();
void init_uart();
void sendch(uint8_t ch);
void initClock_32Mhz();

static RingBuffer_t rxBuffer;
static uint8_t rxBufferData[128];

ISR(USARTD0_RXC_vect)
{ 
	char data = USARTD0.DATA; 
	if(!RingBuffer_IsFull(&rxBuffer))
	{
		RingBuffer_Insert(&rxBuffer, data);
	}
}

int main()
{
	int i = 0;
	initClock_32Mhz();
	init_uart();

	/* Init the ring buffer */
	RingBuffer_InitBuffer(&rxBuffer, rxBufferData, sizeof(rxBufferData));

	/* Onboard LED */
	pinMode(C,7,OUTPUT);

	/* Enable UART data reception interrupt */
	USARTD0.CTRLA |= USART_DRIE_bm;
	
	/* Set UART data reception interrupt priority */
	USARTD0.CTRLA |= (1<<5);
	USARTD0.CTRLA |= (1<<4);
	
	/* Enable all interrupt levels */
	PMIC.CTRL = PMIC_LOLVLEN_bm | PMIC_MEDLVLEN_bm | PMIC_HILVLEN_bm;
	sei();

	while(1)
	{
		/* Just a simple loopback ... */
		if(!RingBuffer_IsEmpty(&rxBuffer))
		{
			sendch(RingBuffer_Remove(&rxBuffer));
		}
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
