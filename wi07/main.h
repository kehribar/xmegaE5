/*-----------------------------------------------------------------------------
/
/
/----------------------------------------------------------------------------*/
#include <avr/io.h>	
#include <string.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "wi07.h"
#include "xprintf.h"
#include "ringBuffer.h"
#include "xmega_digital.h"
#include "hardwareLayer.h"
/*---------------------------------------------------------------------------*/
RingBuffer_t Buffer;
uint8_t tcpData[512];
uint8_t txBuffer[256];
uint8_t smallBuffer[128];
volatile uint8_t BufferData[1024];
/*---------------------------------------------------------------------------*/
uint8_t SSID_name[32];
uint8_t SSID_pass[32];
uint8_t PublicKey[32];
uint8_t PrivateKey[32];
/*---------------------------------------------------------------------------*/
uint8_t* SSID_name_addr = (uint8_t*)32; /* EEPROM base address */
uint8_t* SSID_pass_addr = (uint8_t*)64; /* EEPROM base address */
uint8_t* PublicKey_addr = (uint8_t*)96; /* EEPROM base address */
uint8_t* PrivateKey_addr = (uint8_t*)128; /* EEPROM base address */
/*---------------------------------------------------------------------------*/
int32_t totalSum = 0;
int16_t movingBuffer[8];
uint8_t movingIndex = 0;
uint8_t filterStable = 0;
/*---------------------------------------------------------------------------*/
