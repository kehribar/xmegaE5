/*-----------------------------------------------------------------------------
/
/
/----------------------------------------------------------------------------*/
#include <avr/io.h> 
#include <stdint.h>
#include "xmega_digital.h"
/*---------------------------------------------------------------------------*/
#define spiFlash_cs_low() digitalWrite(C,0,LOW)
#define spiFlash_cs_high() digitalWrite(C,0,HIGH)
#define spiFlash_clk_high() digitalWrite(C,1,HIGH)
#define spiFlash_clk_low() digitalWrite(C,1,LOW)
#define spiFlash_mosi_high() digitalWrite(C,7,HIGH)
#define spiFlash_mosi_low() digitalWrite(C,7,LOW)
#define spiFlash_miso_check() digitalRead(C,6)
/*-----------------------------------------------------------------------------
/ Read data from any arbitrary location 
/----------------------------------------------------------------------------*/
void spiFlash_read_data(uint32_t address, uint32_t length, uint8_t* data);
/*-----------------------------------------------------------------------------
/ The Page Program command allows from one byte to 256 bytes (a page) of data
/ to be programmed at previously erased (FFh) memory locations. 
/----------------------------------------------------------------------------*/
void spiFlash_write_data(uint32_t address, uint32_t length, uint8_t* data);
/*-----------------------------------------------------------------------------
/ The Sector Erase command sets all memory within a specified sector 
/ (4096 bytes) to the erased state of all 1’s (FFh)
/----------------------------------------------------------------------------*/
void spiFlash_sectorErase(uint32_t address);
/*---------------------------------------------------------------------------*/
uint8_t spiFlash_isBusy();
/*---------------------------------------------------------------------------*/
uint8_t spiFlash_check_ID();
/*---------------------------------------------------------------------------*/
void spiFlash_write_enable();
/*---------------------------------------------------------------------------*/
void spiFlash_write_disable();
/*---------------------------------------------------------------------------*/
uint8_t spiFlash_readStatus1();
/*---------------------------------------------------------------------------*/
uint8_t spiFlash_send_spi(uint8_t tx);
/*---------------------------------------------------------------------------*/
