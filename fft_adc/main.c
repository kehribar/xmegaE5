/*-----------------------------------------------------------------------------
/
/
/----------------------------------------------------------------------------*/
#include <avr/io.h>	
#include "xprintf.h"
#include <util/delay.h>
#include "xmega_digital.h"
#include <avr/interrupt.h>
#include "ffft.h"
/*---------------------------------------------------------------------------*/
#define lowByte(word) ((uint16_t)word&0x00FF)
#define highByte(word) (((uint16_t)word&0xFF00)>>8)
/*---------------------------------------------------------------------------*/
#define dma_ch0_running() (EDMA.STATUS & (1<<4))
/*---------------------------------------------------------------------------*/
void init_uart();
void init_adc();
void init_timer();
void init_hardware();
void initClock_32Mhz();
void sendch(uint8_t ch);
void send_uart_dma_ch0();
uint16_t findAdcOffset();
uint16_t read_adc(uint8_t ch);
/*---------------------------------------------------------------------------*/
int16_t* capture;
int16_t* analyse;
int16_t capture1[FFT_N];
int16_t capture2[FFT_N];
complex_t bfly_buff[FFT_N];
uint16_t spektrum[FFT_N/2];
volatile uint8_t acq_done;
volatile int16_t adc_offset;
volatile uint16_t capture_index;
/*---------------------------------------------------------------------------*/
int main()
{
	uint8_t i;
	uint8_t loopCounter = 0;
	
	init_hardware();	

	_delay_ms(3000);

	/* With 20 kHz sampling and 256 point FFT, resolution is ~83 Hz */
	while(1)
	{	
		if(acq_done)
		{
			togglePin(C,6);

			/* Clear the flag */
			acq_done = 0;			

			/* Computing the spektrum with 256 samples takes around 8ms with 32Mhz Clock */	
			fft_input(analyse, bfly_buff);				
			fft_execute(bfly_buff);		
			fft_output(bfly_buff, spektrum);	

			/* Send the data to the PC */
			if((++loopCounter) == 2)
			{
				loopCounter = 0;

				/* If we enter this loop, it means that the UART data transfer is slower than the actual work ... */
				while(dma_ch0_running())
				{
					togglePin(C,7);
				}

				/* Transfer the spektrum buffer */			
				send_uart_dma_ch0((uint8_t*)spektrum,FFT_N);			
			}
		}
	}

	return 0;
}
/*---------------------------------------------------------------------------*/
void init_hardware()
{
	initClock_32Mhz();
	init_uart();
	init_adc();
	init_timer();

	/* Onboard LED */
	pinMode(C,7,OUTPUT);

	/* Debug pin */
	pinMode(C,6,OUTPUT);	

	/* Enable the EDMA module */
	EDMA.CTRL = EDMA_ENABLE_bm;

	/* Set capture paramaters */
	acq_done = 0;
	capture_index = 0;
	capture = capture1;	

	/* Find DC offset value of the analog input */
	adc_offset = findAdcOffset();

	/* Enable all interrupt levels */
    PMIC.CTRL = PMIC_LOLVLEN_bm | PMIC_MEDLVLEN_bm | PMIC_HILVLEN_bm;
	sei();

	digitalWrite(C,7,HIGH);
}
/*---------------------------------------------------------------------------*/
void sendch(uint8_t ch)
{
	while(!(USARTD0.STATUS & USART_DREIF_bm));
	USARTD0.DATA = ch;
}
/*---------------------------------------------------------------------------*/
void init_timer()
{
	/* Each clock tick is: 32.000.000 / prescale Hz */
	/* For 64 => 500 kHz */ 
	TCC5.CTRLA = TC45_CLKSEL_DIV64_gc;

	/* 10 kHz sampling rate */
	TCC5.PER = 49;

	/* Connect Timer Overflow signal to Event CH0 */
	EVSYS.CH0MUX = EVSYS_CHMUX_TCC5_OVF_gc;
}
/*---------------------------------------------------------------------------*/
void init_adc()
{
	/* Use external 2048 mV reference */
	ADCA.REFCTRL = ADC_REFSEL_AREFA_gc;

	/* Set ADC clock prescaler */
	ADCA.PRESCALER = ADC_PRESCALER_DIV8_gc;

	/* Signed mode: GND to AREF = 11 bits resolution */
	ADCA.CTRLB |= (1<<4);

	/* Single ended measurement */
	ADCA.CH0.CTRL = ADC_CH_INPUTMODE_SINGLEENDED_gc;

	/* Enable the ADC */
	ADCA.CTRLA = ADC_ENABLE_bm;

	/* Select ADC input pin as CH1 */
	ADCA.CH0.MUXCTRL = (0x01) << 3;

	/* Select the event channel and activate convert action */
	ADCA.EVCTRL = ADC_EVSEL_0_gc | ADC_EVACT_CH0_gc;

	/* Enable ADC conversion finished interrupt */
	ADCA.CH0.INTCTRL = ADC_CH_INTMODE_COMPLETE_gc | ADC_CH_INTLVL_HI_gc;
	
	/* Averaging mode */
	#if 0
		ADCA.CH0.AVGCTRL = ADC_SAMPNUM_8X_gc | (3 << 4);	
	/* Oversampling mode */
	#else 
		ADCA.CTRLB |= ADC_RESOLUTION_MT12BIT_gc;
		ADCA.CH0.AVGCTRL = ADC_SAMPNUM_16X_gc;
	#endif
}
/*---------------------------------------------------------------------------*/
void init_uart()
{
	/* Set UART pin driver states */
	pinMode(D,6,INPUT);
	pinMode(D,7,OUTPUT);

	/* Remap the UART pins */
	PORTD.REMAP = PORT_USART0_bm;

	USARTD0.CTRLB = USART_RXEN_bm|USART_TXEN_bm;
	USARTD0.CTRLC = USART_CMODE_ASYNCHRONOUS_gc|USART_PMODE_DISABLED_gc|USART_CHSIZE_8BIT_gc;

 	/* 115200 baud rate with 32MHz clock */
    USARTD0.BAUDCTRLA = 131; USARTD0.BAUDCTRLB = (-3 << USART_BSCALE_gp);

	/* Connect the xprintf library to UART hardware */
	xdev_out(sendch);
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
/*---------------------------------------------------------------------------*/
uint16_t findAdcOffset()
{
	uint16_t i;
	uint32_t val = 0;

	for (i = 0; i < 256; i++)
	{
		val += read_adc(1);
	}

	val = val >> 8;

	return val;
}
/*---------------------------------------------------------------------------*/
ISR(ADCA_CH0_vect)
{		
	/* Clear the interrupt flag */
	ADCA.INTFLAGS = ADC_CH0IF_bm;

	if(acq_done == 0)
	{
		/* Store the readout */
		capture[capture_index++] = ADCA.CH0.RES - adc_offset; 	

		/* Change the buffer pointer when one is full */
		if(capture_index == FFT_N)
		{
			if(capture == capture1)
			{
				capture = capture2;
				analyse = capture1;
			}
			else
			{
				capture = capture1;
				analyse = capture2;
			}
			capture_index = 0;
			acq_done = 1;
		}
	}
}
/*---------------------------------------------------------------------------*/
