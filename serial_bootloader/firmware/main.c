/*-----------------------------------------------------------------------------
/ UART bootloader for Atxmega32E5
/------------------------------------------------------------------------------
/ TODO: 
/	- Add Watchdog Timer handling for bootloader recovery.
/	- Restore the 32MHz PLL setting to its original state before exit.
/------------------------------------------------------------------------------
/ <ihsan@kehribar.me> - August 2014
/----------------------------------------------------------------------------*/
#include <avr/io.h>	
#include <util/delay.h>
#include "xmega_digital.h"
#include "sp_driver.h"
/*---------------------------------------------------------------------------*/
int16_t getch();
void init_uart();
void initClock_32Mhz();
void sendch(uint8_t ch);
void (*funcptr)(void) = 0x0000;
/*---------------------------------------------------------------------------*/
/* SPM_PAGESIZE is 128 for Xmega32E5 */
volatile uint8_t pageBuf[SPM_PAGESIZE];
/*---------------------------------------------------------------------------*/
#define VERSION 1
#define getByte() (USARTD0.DATA)
#define newMessage() (USARTD0.STATUS & USART_RXCIF_bm)
/*---------------------------------------------------------------------------*/
static void boot_program_page(uint32_t pageOffset, uint8_t *buf)
{
	SP_LoadFlashPage(buf);
	SP_EraseWriteApplicationPage(pageOffset);
	SP_WaitForSPM();
}
/*---------------------------------------------------------------------------*/
int main(void) 
{    
    uint8_t t8;
    uint16_t i;
    uint8_t msg;
    uint32_t t32;
    uint8_t run = 1;
    uint32_t pageOffset;
    uint32_t counter = 0;

    initClock_32Mhz();
	init_uart();

	/* Onboard LED */
	pinMode(C,7,OUTPUT);
	pinMode(C,6,OUTPUT);	
	digitalWrite(C,6,HIGH);

    /* Wait until a message arrives or timeout */
    while((!newMessage())&&(run))
    {        
        if(counter++ > 0xFFFFF)
        {
            run = 0;
        }        
    }

    /* Was it correct message? */
    if(getByte() != 'a')
    {
        run = 0;
    }
    else
    {
        /* Send ACK */
        sendch('Y');
    }    

    while(run)
    {        
        while(!newMessage());
        msg = getByte();

        togglePin(C,6);
        togglePin(C,7);
        
        switch(msg)
        {
            /* Ping */
            case 'a':
            {
                /* Send ACK */
                sendch('Y');
                break;
            }
            /* Fill the page buffer */
            case 'b':
            {
                /* Send ACK */
                sendch('Y');

                for(i=0;i<SPM_PAGESIZE;i++)
                {
                    while(!newMessage());
                    pageBuf[i] = getByte();
                }

                break;
            }
            /* Program the buffer */
            case 'c':
            {
                /* Send ACK */
                sendch('Y');

                while(!newMessage());
                pageOffset = getByte();

                while(!newMessage());
                t32 = getByte();
                t32 = t32 << 8;
                pageOffset += t32;

                while(!newMessage());
                t32 = getByte();
                t32 = t32 << 16;
                pageOffset += t32;

                while(!newMessage());
                t32 = getByte();
                t32 = t32 << 24;
                pageOffset += t32;

                boot_program_page(pageOffset,pageBuf);

                /* Send ACK */
                sendch('Y');
                break;
            }
            /* Delete the pages */
            case 'd':
            {   
                for(t32=0;t32<BOOTSTART;t32+=SPM_PAGESIZE)
                {
                    SP_EraseApplicationPage(t32);
                    SP_WaitForSPM();            
                }                    

                /* Send ACK */
                sendch('Y');
                break;
            }
            /* Version readout */
            case 'v':
            {
            	sendch(VERSION);
            	break;
            }
            /* Go to user app ... */
            case 'x':
            {
                digitalWrite(C,6,LOW);
    			digitalWrite(C,7,LOW);
    			pinMode(C,6,INPUT);
    			pinMode(C,7,INPUT);
                funcptr();                 
                break;
            }
        }     
    }
    
    digitalWrite(C,6,LOW);
    digitalWrite(C,7,LOW);
	pinMode(C,6,INPUT);
	pinMode(C,7,INPUT);

    /* Go to user app ... */
    funcptr();

    return 0;
}
/*---------------------------------------------------------------------------*/
void sendch(uint8_t ch)
{
	while(!(USARTD0.STATUS & USART_DREIF_bm));

	USARTD0.DATA = ch;
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