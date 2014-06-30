/*-----------------------------------------------------------------------------
/ Basic ADC example for XmegaE5 microcontroller.
/------------------------------------------------------------------------------
/ - Uses 2048 mV external reference connected to the A0 pin
/ - Reads the ADC in "blocking" fashion. 
/ - Uses single ended & signed conversion mode. Resolution is 11 bits.
/ - Each LSB corresponds to 1mV.
/ - No calibration routines are implemented for the sake of simplicity.
/------------------------------------------------------------------------------
/ <ihsan@kehribar.me> - July 2014
/----------------------------------------------------------------------------*/
#include <avr/io.h>	
#include "xprintf.h"
#include <util/delay.h>
#include "xmega_digital.h"

int16_t getch();
void init_adc();
void init_uart();
void initClock_32Mhz();
void sendch(uint8_t ch);
uint16_t read_adc(uint8_t ch);

int main()
{
	int16_t tmp;

	initClock_32Mhz();
	init_adc();
	init_uart();

	/* Onboard LED */
	pinMode(C,7,OUTPUT);
	togglePin(C,7);
	_delay_ms(100);
	togglePin(C,7);

	xprintf("Hi!\r\n");

	while(1)
	{
		xprintf("----------------------\r\n");

		tmp = read_adc(1);
		xprintf("[1]: %d mV\r\n",tmp);

		tmp = read_adc(2);
		xprintf("[2]: %d mV\r\n",tmp);

		tmp = read_adc(3);
		xprintf("[3]: %d mV\r\n",tmp);
		
		_delay_ms(500);
	}
}

void init_adc()
{
	/* Use external 2048 mV reference */
	ADCA.REFCTRL = ADC_REFSEL_AREFA_gc;

	/* Set ADC clock prescaler */
	ADCA.PRESCALER = ADC_PRESCALER_DIV512_gc;

	/* Signed mode: GND to AREF = 11 bits resolution */
	ADCA.CTRLB |= (1<<4);

	/* Single ended measurement */
	ADCA.CH0.CTRL = ADC_CH_INPUTMODE_SINGLEENDED_gc;

	/* Enable the ADC */
	ADCA.CTRLA = ADC_ENABLE_bm;
}	

uint16_t read_adc(uint8_t ch)
{
	/* Select ADC input pin */
	ADCA.CH0.MUXCTRL = ch << 3;
	
	/* Start conversion and wait until it is done ... */
	ADCA.CH0.CTRL |= ADC_CH_START_bm;
	while(!(ADCA.CH0.INTFLAGS & ADC_CH0IF_bm)); 
	ADCA.INTFLAGS = ADC_CH0IF_bm;
	
	/* Return the result */
	return ADCA.CH0.RES;
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
