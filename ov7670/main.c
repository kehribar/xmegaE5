/*-----------------------------------------------------------------------------
/
/
/----------------------------------------------------------------------------*/
#include <avr/io.h>	
#include "xprintf.h"
#include <util/delay.h>
#include "xmega_digital.h"
#include "bitbang_i2c.h"
#include "ov7670reg.h"
#include <avr/interrupt.h>
/*---------------------------------------------------------------------------*/
void init_uart();
void ov7670_init();
void init_clockout();
void initClock_32Mhz();
void sendch(uint8_t ch);
void store_pixels_dma_ch2();
uint8_t ov7670_get(uint8_t addr);
uint8_t ov7670_set(uint8_t addr, uint8_t val);
void send_uart_dma_ch0(uint8_t* buf, uint16_t len);
/*---------------------------------------------------------------------------*/
uint8_t linebuffer1[64];
uint8_t linebuffer2[64];
uint8_t* linebuffer = linebuffer1;
/*---------------------------------------------------------------------------*/
#define dma_ch0_running() (EDMA.CH0.CTRLA & EDMA_CH_ENABLE_bm)
#define dma_ch2_running() (EDMA.CH2.CTRLA & EDMA_CH_ENABLE_bm)
/*---------------------------------------------------------------------------*/
volatile uint8_t line_index = 0;
/*---------------------------------------------------------------------------*/
#define WIDTH_MAX 160
#define HEIGHT_MAX 120
volatile uint8_t width_scale = 2; /* minimum: 2 */
volatile uint8_t height_scale = 2; /* minimum: 2 */
/*---------------------------------------------------------------------------*/
int main()
{		
	initClock_32Mhz();
	
    init_uart();

	ov7670_init();

    /*-----------------------------------------------------------------------*/

    /* Enable the EDMA module */
    /* 0,1 are peripheral channels. 2 is standard channel */
    EDMA.CTRL = EDMA_ENABLE_bm | EDMA_CHMODE_STD2_gc; 

    /*-----------------------------------------------------------------------*/

    /* Port C - Data pins - All inputs */
    PORTC.DIR = 0x00;

    pinMode(D,1,INPUT); // HSYNC
    PORTD.PIN1CTRL |= PORT_ISC_FALLING_gc; // falling edge on hsync indicates end of line
    
    pinMode(D,0,INPUT); // VSYNC
    PORTD.PIN0CTRL |= PORT_ISC_FALLING_gc; // falling edge on vsync indicates end of an image 

    /* Enable interrupts on the PORTD */
    PORTD.INTCTRL = PORT_INTLVL_HI_gc;
    PORTD.INTMASK = (1<<1) | (1<<0);

    /*-----------------------------------------------------------------------*/ 

    pinMode(A,5,OUTPUT); // Debug pin A5
    pinMode(A,4,OUTPUT); // Debug pin A4
    pinMode(A,3,OUTPUT); // Debug pin A3

    /*-----------------------------------------------------------------------*/    

    pinMode(A,7,INPUT); // PCLK
    PORTA.PIN7CTRL |= PORT_ISC_FALLING_gc; // rising edge pixel clock
    EVSYS.CH0MUX = EVSYS_CHMUX_PORTA_PIN7_gc;

    TCC4.CCA = 0;
    TCC4.CTRLE = (1<<0);
    TCC4.PER = (2 * width_scale) - 1;
    TCC4.CTRLA = TC45_CLKSEL_EVCH0_gc;      
    EVSYS.CH1MUX = EVSYS_CHMUX_TCC4_CCA_gc;    

    /*-----------------------------------------------------------------------*/    

    pinMode(R,0,OUTPUT);
    PORTCFG.ACEVOUT = (1<<0) | (1<<3) | (1<<4) | (1<<5);

    /*-----------------------------------------------------------------------*/    

    /* Enable all interrupt levels */
    PMIC.CTRL = PMIC_LOLVLEN_bm | PMIC_MEDLVLEN_bm | PMIC_HILVLEN_bm;    

    /* Clear all previous interrupt flags */
    PORTD.INTFLAGS = 0xFF;

    sei(); 

    /*-----------------------------------------------------------------------*/    

	while(1)
	{	        
        /* Empty main loop ... */
	}

    /*-----------------------------------------------------------------------*/    
}
/*---------------------------------------------------------------------------*/
ISR(PORTD_INT_vect)
{
    /* End of a single line */
    if(PORTD.INTFLAGS & (1<<1))
    {                  
        if(line_index == (height_scale -1))
        {                              
            line_index = 0;      
            TCC4.CNT = 0x00;              
            TCC4.CTRLA = TC45_CLKSEL_EVCH0_gc;              
            store_pixels_dma_ch2(linebuffer,WIDTH_MAX / width_scale);            
        }
        else
        {
            if(line_index == 0)
            {
                if(dma_ch0_running() == 0)
                {
                    digitalWrite(A,3,LOW);
                    send_uart_dma_ch0(linebuffer,WIDTH_MAX / width_scale);  
                }                         
                else
                {                    
                    digitalWrite(A,3,HIGH);
                }                     
            } 

            line_index++;            
            TCC4.CTRLA = 0x00;
            TCC4.CNT = 0x00;     
        }
        
        PORTD.INTFLAGS = (1<<1);
    }
    /* End of a complete image */
    else if(PORTD.INTFLAGS & (1<<0))
    {
        line_index = 0;      
        TCC4.CNT = 0x00;   

        /* Send preamble for the image data */   
        sendch(0xAA);
        sendch(0x55);
        sendch(0xAA);     

        PORTD.INTFLAGS = (1<<0); 
    }   
}
/*---------------------------------------------------------------------------*/
void sendch(uint8_t ch)
{
	while(!(USARTD0.STATUS & USART_DREIF_bm));
	USARTD0.DATA = ch;
}
/*---------------------------------------------------------------------------*/
void init_clockout()
{
    /* Clock out pin */
	pinMode(D,4,OUTPUT);
    digitalWrite(D,4,HIGH);

    /* Each clock tick is: 32.000.000 / prescale Hz */
    TCD5.CTRLA = TC45_CLKSEL_DIV1_gc;

    /* Frequency mode */
    TCD5.CTRLB = 0x01;

    /* 16 / (n+1) = Output frequency */
    TCD5.CCA = 0;

    /* Output compare enabled for CCA */
    TCD5.CTRLE = (1<<0);
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

    #if 0
        /* 115200 baud rate with 32MHz clock */
        USARTD0.BAUDCTRLA = 131; USARTD0.BAUDCTRLB = (-3 << USART_BSCALE_gp);
    #else
        /* 230400 baud rate with 32MHz clock */
        // USARTD0.BAUDCTRLA = 123; USARTD0.BAUDCTRLB = (-4 << USART_BSCALE_gp);       
        USARTD0.BAUDCTRLA = 0x00; USARTD0.BAUDCTRLB = 0x00;
    #endif

	/* Connect the xprintf library to UART hardware */
	xdev_out(sendch);
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
void ov7670_init()
{
    I2C_Init();
    init_clockout();

    /* Reset pin */
    pinMode(D,5,OUTPUT);
    digitalWrite(D,5,LOW);  _delay_ms(100);
    digitalWrite(D,5,HIGH); _delay_ms(100);

    ov7670_set(REG_COM7, 0x80); /* reset to default values */
    _delay_ms(100);
    ov7670_set(REG_COM7, 0x00); /* reset to default values */
    _delay_ms(100);

    ov7670_set(REG_COM11, 0x0A);
    ov7670_set(REG_TSLB, 0x04);
    ov7670_set(REG_TSLB, 0x04);
    
    ov7670_set(REG_HSTART, 0x16);
    ov7670_set(REG_HSTOP, 0x04);
    ov7670_set(REG_HREF, 0x24);
    ov7670_set(REG_VSTART, 0x02);
    ov7670_set(REG_VSTOP, 0x7a);
    ov7670_set(REG_VREF, 0x0a);
    ov7670_set(REG_COM10, 0x02 + (1<<5));
    ov7670_set(REG_COM3, 0x04);
    ov7670_set(REG_MVFP, 0x27);

    #if 1
        ov7670_set(REG_COM14, 0x1a); // divide by 4
        ov7670_set(0x72, 0x22); // downsample by 4    
        ov7670_set(0x73, 0xf2); // divide by 4
    #else
        ov7670_set(REG_COM14, 0x1b); // divide by 8
        ov7670_set(0x72, 0x33); // downsample by 8
        ov7670_set(0x73, 0xf3); // divide by 8
    #endif

    #if 0
        // COLOR SETTING
        ov7670_set(0x4f, 0x80);
        ov7670_set(0x50, 0x80);
        ov7670_set(0x51, 0x00);
        ov7670_set(0x52, 0x22);
        ov7670_set(0x53, 0x5e);
        ov7670_set(0x54, 0x80);
        ov7670_set(0x56, 0x40);
        ov7670_set(0x58, 0x9e);
        ov7670_set(0x59, 0x88);
        ov7670_set(0x5a, 0x88);
        ov7670_set(0x5b, 0x44);
        ov7670_set(0x5c, 0x67);
        ov7670_set(0x5d, 0x49);
        ov7670_set(0x5e, 0x0e);
        ov7670_set(0x69, 0x00);
        ov7670_set(0x6a, 0x40);
        ov7670_set(0x6b, 0x0a);
        ov7670_set(0x6c, 0x0a);
        ov7670_set(0x6d, 0x55);
        ov7670_set(0x6e, 0x11);
        ov7670_set(0x6f, 0x9f);
        ov7670_set(0xb0, 0x84);
    #endif    

}
/*---------------------------------------------------------------------------*/
uint8_t ov7670_set(uint8_t addr, uint8_t val)
{   
    int rc;

    I2C_Start();
        rc = I2C_Write(write_address(0x21));
        rc = I2C_Write(addr);
        rc = I2C_Write(val);
    I2C_Stop();

    return rc;
}
/*---------------------------------------------------------------------------*/
uint8_t ov7670_get(uint8_t addr)
{
    int rc;
    uint8_t data;

    I2C_Start();
        rc = I2C_Write(write_address(0x21));
        rc = I2C_Write(addr);
    I2C_Stop();

    _delay_ms(1);

    I2C_Start();
        rc = I2C_Write(read_address(0x21));
        data = I2C_Read(NO_ACK);
    I2C_Stop();

    return data;
}
/*---------------------------------------------------------------------------*/
void store_pixels_dma_ch2(uint8_t* buf, uint16_t len)
{
     /* Source address */
    EDMA.CH2.ADDR = 0x16;

    /* Transfer size */
    EDMA.CH2.TRFCNT = len;

    /* Destination address */
    EDMA.CH2.DESTADDR = buf;

    /* Destination Memory address mode: Increment */
    EDMA.CH2.DESTADDRCTRL = EDMA_CH_DIR_INC_gc;

    /* Trigger source */
    EDMA.CH2.TRIGSRC = EDMA_CH_TRIGSRC_EVSYS_CH1_gc;

    /* Single shot mode */
    EDMA.CH2.CTRLA = EDMA_CH_SINGLE_bm;    

    /* Enable the channel #2 ! */
    EDMA.CH2.CTRLA |= EDMA_CH_ENABLE_bm;
}
/*---------------------------------------------------------------------------*/
void send_uart_dma_ch0(uint8_t* buf, uint16_t len)
{   
    /* Source address */
    EDMA.CH0.ADDR = buf; 

    /* Transfer size */
    EDMA.CH0.TRFCNT = len;

    /* Trigger source: Data register empty */
    EDMA.CH0.TRIGSRC = EDMA_CH_TRIGSRC_USARTD0_DRE_gc;

    /* Memory address mode: Increment */
    EDMA.CH0.ADDRCTRL = EDMA_CH_DIR_INC_gc;

    /* Single shot mode */
    EDMA.CH0.CTRLA = EDMA_CH_SINGLE_bm;

    /* Enable the channel #0 ! */
    EDMA.CH0.CTRLA |= EDMA_CH_ENABLE_bm;
}
/*---------------------------------------------------------------------------*/
