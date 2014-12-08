/*-----------------------------------------------------------------------------
/
/
/----------------------------------------------------------------------------*/
#include "main.h"
/*---------------------------------------------------------------------------*/
void init_adc();
void init_uart();
void fail_if_not_ok();
void initClock_32Mhz();
void sendch(uint8_t ch);
uint16_t read_adc(uint8_t ch);
/*---------------------------------------------------------------------------*/
int main()
{
	initClock_32Mhz();
	init_uart();
	init_adc();

	RingBuffer_InitBuffer(&Buffer, BufferData, sizeof(BufferData));

	/* Software PWM timer */	 
    TCC4.CTRLA = TC45_CLKSEL_DIV256_gc; /* 32Mhz / 256 => 125 kHz */
    TCC4.PER = 9; /* 125 kHz / (9+1) => 12.5 kHz */
    TCC4.INTCTRLA = 0x01; /* Overflow interrupt level */

	/* Wifi module reset / control pins */
	pinMode(C,4,OUTPUT);
	digitalWrite(C,4,HIGH);
	
	pinMode(C,5,OUTPUT);
	digitalWrite(C,5,HIGH);

	/* Onboard LEDs */
	pinMode(C,7,OUTPUT);
	pinMode(C,6,OUTPUT);

	/* Button 0 */
	pinMode(D,3,INPUT);
	setInternalPullup(D,3);

	/* RGB LED */
	pinMode(D,0,OUTPUT);
	pinMode(D,1,OUTPUT);
	pinMode(D,2,OUTPUT);
		
	set_rgb(255,0,0);	

	xsprintf(SSID_name,"name");
	xsprintf(SSID_pass,"pass");
	
	/* Wait for the powerup */
	wait_for_message("ready\r\n",10000);

	/* Enter AP mode for configuration */
	if(digitalRead(D,3) == LOW)
	{
		xprintf("AT+CWMODE=2\r\n");
		check_ok();

		xprintf("AT+RST\r\n");
		check_ok();

		/* Wait for the powerup */
		wait_for_message("ready\r\n",10000);

		/* SSID details ... */
		xprintf("AT+CWSAP=\"small_device\",\"password\",5,3\r\n");

		/* Check the result of join */
		fail_if_not_ok();

		set_rgb(255,0,255);
	}
	else
	{
		xprintf("AT+CWMODE=1\r\n");
		check_ok();

		xprintf("AT+RST\r\n");
		check_ok();

		/* Wait for the powerup */
		wait_for_message("ready\r\n",10000);
	
		/* SSID details ... */
		xprintf("AT+CWJAP=\"");
		xprintf("%s\",\"",SSID_name);
		xprintf("%s\"\r\n",SSID_pass);

		/* Check the result of join */
		fail_if_not_ok();

		set_rgb(255,255,0);
	}

	/* Wait until system get IP */
	do
	{
		xprintf("AT+CIFSR\r\n");
		_delay_ms(100);	
	}
	while(check_error() == 0);	
	
	/* ... */
	xprintf("AT+CIPMUX=1\r\n");
	fail_if_not_ok();
	
	/* Enable server at port 80 */
	xprintf("AT+CIPSERVER=1,80\r\n");
	fail_if_not_ok();		

	uint16_t len;
	uint8_t ind = 0;	

	while(1)
	{
		ind = 0;

		len = waitTCPMessage_blocking();

		switch(tcpData[0])
		{
			/* SSID name */
			case 's':
			{
				while(tcpData[ind+2] != '\r')
				{
					SSID_name[ind] = tcpData[ind+2];
					ind++;				
				}

				SSID_name[ind] = '\0';

				xsprintf(tmpBuffer,"> SSID name set: %s\r\n",SSID_name);

				send_TCPData(tmpBuffer,strlen(tmpBuffer));

				break;
			}	
			/* SSID  password */
			case 'p':
			{
				while(tcpData[ind+2] != '\r')
				{
					SSID_pass[ind] = tcpData[ind+2];
					ind++;				
				}

				SSID_name[ind-1] = '\0';

				xsprintf(tmpBuffer,"> SSID pass set: %s\r\n",SSID_pass);

				send_TCPData(tmpBuffer,strlen(tmpBuffer));
				
				break;
			}	
			/* Get temperature */
			case 't':
			{
				xsprintf(tmpBuffer,"> Temperature: %d\r\n",read_adc(1));
				
				send_TCPData(tmpBuffer,strlen(tmpBuffer));
				
				break;
			}
			/* Unknown command */
			default:
			{
				send_TCPData("> Err!\r\n",8);

				break;
			}
		}	
	}
}
/*---------------------------------------------------------------------------*/
void fail_if_not_ok()
{
	if(check_ok() != 0)
	{
		while(1)
		{
			set_rgb(255,0,0);
			_delay_ms(100);
			set_rgb(0,255,0);
			_delay_ms(100);
			set_rgb(0,0,255);
			_delay_ms(100);
		}
	}
}
/*---------------------------------------------------------------------------*/
ISR(USARTC0_RXC_vect)
{ 
	char data = USARTC0.DATA; 

	if(!RingBuffer_IsFull(&Buffer))
	{
		RingBuffer_Insert(&Buffer,data);
	}
}
/*---------------------------------------------------------------------------*/
/* Software PWM routine */
ISR(TCC4_OVF_vect)
{
	if(r_counter++ >= r)
	{
		digitalWrite(D,2,HIGH);
	}
	else
	{
		digitalWrite(D,2,LOW);
	}

	if(g_counter++ >= g)
	{
		digitalWrite(D,1,HIGH);
	}
	else
	{
		digitalWrite(D,1,LOW);
	}

	if(b_counter++ >= b)
	{
		digitalWrite(D,0,HIGH);
	}
	else
	{
		digitalWrite(D,0,LOW);
	}

	TCC4.INTFLAGS |= (1<<0);
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
void sendch(uint8_t ch)
{
    while(!(USARTC0.STATUS & USART_DREIF_bm));
    USARTC0.DATA = ch;
}
/*---------------------------------------------------------------------------*/
void init_uart()
{
	pinMode(C,2,INPUT);
	pinMode(C,3,OUTPUT);

	USARTC0.CTRLB = USART_RXEN_bm|USART_TXEN_bm;
	USARTC0.CTRLC = USART_CMODE_ASYNCHRONOUS_gc|USART_PMODE_DISABLED_gc|USART_CHSIZE_8BIT_gc;

 	/* 115200 baud rate with 32MHz clock */
    USARTC0.BAUDCTRLA = 131; USARTC0.BAUDCTRLB = (-3 << USART_BSCALE_gp);

    /* Enable UART data reception interrupt */
	USARTC0.CTRLA |= USART_DRIE_bm;
	
	/* Set UART data reception interrupt priority */
	USARTC0.CTRLA |= (1<<5);
	USARTC0.CTRLA |= (1<<4);
  
   	/* Enable all interrupt levels */
    PMIC.CTRL = PMIC_LOLVLEN_bm | PMIC_MEDLVLEN_bm | PMIC_HILVLEN_bm;
	sei();

	/* Connect the xprintf library to UART hardware */
	xdev_out(sendch);
}
/*---------------------------------------------------------------------------*/
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