/*-----------------------------------------------------------------------------
/
/
/----------------------------------------------------------------------------*/
#include <avr/io.h>	
#include <string.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "wi07.h"
#include "xprintf.h"
#include "ringBuffer.h"
#include "xmega_digital.h"
/*---------------------------------------------------------------------------*/
RingBuffer_t Buffer;
uint8_t tcpData[512];
volatile uint8_t BufferData[1024];
/*---------------------------------------------------------------------------*/
uint8_t SSID_name[64];
uint8_t SSID_pass[64];
/*---------------------------------------------------------------------------*/
uint8_t tmpBuffer[256];
/*---------------------------------------------------------------------------*/
volatile uint8_t r = 0;
volatile uint8_t g = 0;
volatile uint8_t b = 0;
volatile uint8_t r_counter = 0;
volatile uint8_t g_counter = 0;
volatile uint8_t b_counter = 0;
#define set_rgb(rval,gval,bval) {r=rval; g=gval; b=bval;}
/*---------------------------------------------------------------------------*/
