/*-----------------------------------------------------------------------------
/
/
/----------------------------------------------------------------------------*/
#include "spiFlash.h"

uint8_t spiFlash_send_spi(uint8_t tx)
{
  uint8_t i;
  uint8_t rx = 0;

  for(i=0;i<8;i++)
  {
    rx = rx << 1;

    if(tx & 0x80)
    {
      spiFlash_mosi_high();     
    }
    else
    {
      spiFlash_mosi_low();
    }

    spiFlash_clk_high();    
    spiFlash_clk_low();   

    if(spiFlash_miso_check())
    {
      rx |= 0x01;
    }

    tx = tx << 1;
  }

  return rx;
}

uint8_t spiFlash_check_ID()
{
  uint8_t capacity;
  uint8_t device_type;
  uint8_t manufacturer;
    
  spiFlash_cs_low();

  /* 'Read JEDEC ID' command */
  spiFlash_send_spi(0x9F);
  
  manufacturer = spiFlash_send_spi(0x00);
  device_type = spiFlash_send_spi(0x00);
  capacity = spiFlash_send_spi(0x00); 
  
  spiFlash_cs_high(); 

  /* SPANSION SPI Flash: S25FL164K */
  if(((manufacturer == 0x01) && (device_type == 0x40)) && (capacity == 0x17))
  {
    return 1; /* OK */
  }
  else
  {
    return 0; /* FAIL */
  }
}

/*
  Read data from any arbitrary location 
*/
void spiFlash_read_data(uint32_t address, uint32_t length, uint8_t* data)
{
  uint32_t tmp;

  spiFlash_cs_low();
  
  /* Normal speed 'Read Data' command */  
  spiFlash_send_spi(0x03); 
  
  /* A23-A16 */
  tmp = ((address >> 16) & 0xFF); 
  spiFlash_send_spi(tmp); 
  
  /* A15-A8 */
  tmp = ((address >> 8) & 0xFF); 
  spiFlash_send_spi(tmp); 
  
  /* A7-A0 */
  tmp = ((address >> 0) & 0xFF); 
  spiFlash_send_spi(tmp); 

  for(tmp=0; tmp < length; tmp++)
  {
    /* Fetch data one byte at a time ... */
    data[tmp] = spiFlash_send_spi(0x00);  
  }

  spiFlash_cs_high();
}

void spiFlash_write_enable()
{
  spiFlash_cs_low();
  spiFlash_send_spi(0x06);
  spiFlash_cs_high();
}

void spiFlash_write_disable()
{
  spiFlash_cs_low();
  spiFlash_send_spi(0x04);
  spiFlash_cs_high();
}

/* 
  The Page Program command allows from one byte to 256 bytes (a page) of data
  to be programmed at previously erased (FFh) memory locations. 
*/
void spiFlash_write_data(uint32_t address, uint32_t length, uint8_t* data)
{
  uint32_t tmp;

  spiFlash_cs_low();
  
  /* 'Page Program' command */  
  spiFlash_send_spi(0x02);
  
  /* A23-A16 */
  tmp = ((address >> 16) & 0xFF); 
  spiFlash_send_spi(tmp); 
  
  /* A15-A8 */
  tmp = ((address >> 8) & 0xFF); 
  spiFlash_send_spi(tmp); 
  
  /* A7-A0 */
  tmp = ((address >> 0) & 0xFF); 
  spiFlash_send_spi(tmp); 

  for(tmp=0; tmp < length; tmp++)
  {
    /* Send data one byte at a time ... */
    spiFlash_send_spi(data[tmp]);   
  } 

  spiFlash_cs_high(); 
}

/*
  The Sector Erase command sets all memory within a specified sector 
  (4096 bytes) to the erased state of all 1’s (FFh)
*/
void spiFlash_sectorErase(uint32_t address)
{
  uint32_t tmp;

  spiFlash_cs_low();
  
  /* 'Sector Erase' command */  
  spiFlash_send_spi(0x20);
  
  /* A23-A16 */
  tmp = ((address >> 16) & 0xFF); 
  spiFlash_send_spi(tmp); 
  
  /* A15-A8 */
  tmp = ((address >> 8) & 0xFF); 
  spiFlash_send_spi(tmp); 
  
  /* A7-A0 */
  tmp = ((address >> 0) & 0xFF); 
  spiFlash_send_spi(tmp);   

  spiFlash_cs_high();
}

uint8_t spiFlash_readStatus1()
{
  uint8_t tmp;

  spiFlash_cs_low();
  spiFlash_send_spi(0x05);
  tmp = spiFlash_send_spi(0x00);
  spiFlash_cs_high();

  return tmp;
}

uint8_t spiFlash_isBusy()
{
  if(spiFlash_readStatus1() & 0x01)
  {
    return 1;
  }
  else
  {
    return 0;
  }
}