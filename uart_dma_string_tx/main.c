/*-----------------------------------------------------------------------------
/ Bareboes EDMA example with UART
/------------------------------------------------------------------------------
/ Uses EDMA Channel0 for UART transmission.
/------------------------------------------------------------------------------
/ <ihsan@kehribar.me> - July 2014
/----------------------------------------------------------------------------*/
#include <avr/io.h>	
#include <util/delay.h>
#include "xmega_digital.h"
/*---------------------------------------------------------------------------*/
void init_uart();
void initClock_32Mhz();
void send_uart_dma_ch0(uint8_t* buf, uint16_t len);
/*---------------------------------------------------------------------------*/
static uint8_t* dmaTestString = "Hello World!\r\n";
/*---------------------------------------------------------------------------*/
#define lowByte(word) ((uint16_t)word&0x00FF)
#define highByte(word) (((uint16_t)word&0xFF00)>>8)
/*---------------------------------------------------------------------------*/
#define dma_ch0_running() (EDMA.STATUS & (1<<4))
/*---------------------------------------------------------------------------*/
int main()
{
	uint8_t rx;

	initClock_32Mhz();

	init_uart();

	/* Onboard LED */
	pinMode(C,7,OUTPUT);

	/* Enable the complete EDMA peripheral */
	EDMA.CTRL = EDMA_ENABLE_bm;

	while(1)
	{
		togglePin(C,7);
		send_uart_dma_ch0(dmaTestString,14); 
		while(dma_ch0_running());				
	}
}
/*---------------------------------------------------------------------------*/
void send_uart_dma_ch0(uint8_t* buf, uint16_t len)
{	
	/* Source address */
	EDMA.CH0.ADDRL = lowByte(buf);
	EDMA.CH0.ADDRH = highByte(buf);

	/* Transfer size */
	EDMA.CH0.TRFCNTL = lowByte(len);
	EDMA.CH0.TRFCNTH = highByte(len);

	/* Destination address */
	EDMA.CH0.DESTADDRL = lowByte(USARTD0.DATA);
	EDMA.CH0.DESTADDRH = highByte(USARTD0.DATA);

	/* Trigger source: Data register empty */
	EDMA.CH0.TRIGSRC = EDMA_CH_TRIGSRC_USARTD0_DRE_gc;

	/* Memory address mode: Increment */
	EDMA.CH0.ADDRCTRL = EDMA_CH_DIR_INC_gc;

	/* Single shot mode */
	EDMA.CH0.CTRLA |= EDMA_CH_SINGLE_bm;

	/* Enable the channel #0 ! */
	EDMA.CH0.CTRLA |= EDMA_CH_ENABLE_bm;
}
/*---------------------------------------------------------------------------*/
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
/*---------------------------------------------------------------------------*/
void initClock_32Mhz()
{
	/* Generates 32Mhz clock from internal 2Mhz clock via PLL */
	OSC.PLLCTRL = OSC_PLLSRC_RC2M_gc | 16;
	OSC.CTRL |= OSC_PLLEN_bm ;
	while((OSC.STATUS & OSC_PLLRDY_bm) == 0);
	CCP = CCP_IOREG_gc;
	CLK.CTRL = CLK_SCLKSEL_PLL_gc;
}
/*---------------------------------------------------------------------------*/
