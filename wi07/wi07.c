/*-----------------------------------------------------------------------------
/
/
/----------------------------------------------------------------------------*/
#include "wi07.h"
/*---------------------------------------------------------------------------*/
int8_t wait_for_message(const char* checkmsg,uint32_t timeoutLimit)
{
  uint8_t ch;
  uint16_t in = 0;
  uint8_t run = 1;
  uint32_t timeout = 0;
  
  while(run == 1)
  {
    while(RingBuffer_GetCount(&Buffer) == 0)
    {
      if(timeoutLimit != 0)
      {
        timeout++;
        _delay_us(750);
        if(timeout > timeoutLimit) { return -1; }
      }
    } 

    ch = RingBuffer_Remove(&Buffer);

    if(ch == checkmsg[in])
    {
      in++;
    }
    else
    {
      if(timeoutLimit != 0)
      {
        timeout++;
        _delay_us(750);
        if(timeout > timeoutLimit) { return -1; }
      }
    }

    if(checkmsg[in] == '\0')
    {
      run = 0;
    }
  }

  return 0;
}
/*---------------------------------------------------------------------------*/
int8_t check_ok()
{
  uint8_t ch;
  uint8_t run = 1;
  uint16_t in = 0;
  uint32_t timeout = 0; 
  uint8_t ok_buf[4] = {'O','K','\r','\n'};
  
  while(run == 1)
  {
    while(RingBuffer_GetCount(&Buffer) == 0)
    {
      timeout++;
      _delay_us(750);
      if(timeout > 10000) { return -1; }      
    }     

    ch = RingBuffer_Remove(&Buffer);

    if(ch == ok_buf[in])
    {
      in++;
    }
    else
    {
      timeout++;
      _delay_us(750);
      if(timeout > 10000) { return -1; }
    }

    if(in == 4)
    {
      run = 0;
    }
  }

  return 0;
}
/*---------------------------------------------------------------------------*/
void send_TCPData(uint8_t* msg,uint8_t len)
{
  xprintf("AT+CIPSEND=0,%d\r\n",len);

  wait_for_message("> ",10000);

  xprintf("%s",msg);

  fail_if_not_ok();
}
/*---------------------------------------------------------------------------*/
int8_t check_busy()
{
  uint8_t ch;
  uint8_t run = 1;
  uint16_t in = 0;
  uint32_t timeout = 0; 
  uint8_t ok_buf[6] = {'b','u','s','y','\r','\n'};
  
  while(run == 1)
  {
    while(RingBuffer_GetCount(&Buffer) == 0)
    {
      timeout++;
      _delay_us(100);
      if(timeout > 10000) { return -1; }
    }     

    ch = RingBuffer_Remove(&Buffer);

    if(ch == ok_buf[in])
    {
      in++;
    }
    else
    {
      timeout++;
      _delay_us(100);
      if(timeout > 10000) { return -1; }
    }

    if(in == 6)
    {
      run = 0;
    }
  }

  return 0;
}
/*---------------------------------------------------------------------------*/
int8_t check_error()
{
  uint8_t ch;
  uint8_t run = 1;
  uint16_t in = 0;
  uint32_t timeout = 0; 
  uint8_t ok_buf[7] = {'E','R','R','O','R','\r','\n'};
  
  while(run == 1)
  {
    while(RingBuffer_GetCount(&Buffer) == 0)
    {
      timeout++;
      _delay_us(100);
      if(timeout > 10000) { return -1; }
    }     

    ch = RingBuffer_Remove(&Buffer);

    if(ch == ok_buf[in])
    {
      in++;
    }
    else
    {
      timeout++;
      _delay_us(100);
      if(timeout > 10000) { return -1; }
    }

    if(in == 7)
    {
      run = 0;
    }
  }

  return 0;
}
/*---------------------------------------------------------------------------*/
uint16_t getMessageLength()
{
  uint8_t in = 0;
  uint8_t run = 1;  
  uint8_t lenbuf[16];

  while(run)
  {
    while(RingBuffer_GetCount(&Buffer) == 0);
    
    lenbuf[in] = RingBuffer_Remove(&Buffer);

    if(lenbuf[in] == ':')
    {
      run = 0;
      lenbuf[in] = '\0';
    }
    else
    {
      in++;
    }
  }
  
  return StrTo16Uint(lenbuf);
}
/*---------------------------------------------------------------------------*/
/* Fill tcpData array, return message length */
uint16_t waitTCPMessage_blocking()
{
  uint16_t len;
  wait_for_message("+IPD,",0);
  wait_for_message(",",0);
  len = getMessageLength(); 
  
  uint16_t t16;
  for(t16=0;t16 < len;t16++)
  {
    while(RingBuffer_GetCount(&Buffer) == 0);
    tcpData[t16] = RingBuffer_Remove(&Buffer);
  }

  wait_for_message("OK\r\n",10000);

  return len;
}
/*---------------------------------------------------------------------------*/
/* taken from: https://github.com/cnlohr/wi07clight/blob/master/avr_firmware/util10.c */
/*---------------------------------------------------------------------------*/
uint16_t StrTo16Uint(char * str)
{
  uint16_t ret = 0;
  uint8_t yet = 0;
  char c;
  while((c = (*(str++))))
  {
    if((c >= '0')&&(c <= '9'))
    {
      yet = 1;
      ret = ret * 10 + (c - '0');
    }
    else if( yet )
    {
      //Chars in the middle of the number.
      return ret;
    }
  }
  return ret;
}
/*-----------------------------------------------------------------------------
/ fill a binary string of len data into the tcp packet
/ taken from tuxgraphics ip stack
/----------------------------------------------------------------------------*/
uint16_t fill_tcp_data_len(uint8_t *buf,uint16_t pos, const uint8_t *s, uint8_t len)
{
  // fill in tcp data at position pos
  while (len) 
  {
    buf[pos]=*s;
    pos++;
    s++;
    len--;
  }
  return(pos);
}
/*-----------------------------------------------------------------------------
/ fill in tcp data at position pos. pos=0 means start of
/ tcp data. Returns the position at which the string after
/ this string could be filled.
/ taken from tuxgraphics ip stack
/----------------------------------------------------------------------------*/
uint16_t fill_tcp_data(uint8_t *buf,uint16_t pos, const char *s)
{
  return(fill_tcp_data_len(buf,pos,(uint8_t*)s,strlen(s)));
}
/*-----------------------------------------------------------------------------
/ Tries to parse arguments like:
/  http://192.168.4.1/s,bu_bir_internet_agidir
/ and saves that string to proper EEPROM location
/------------------------------------------------------------------------------
/ tcpData[0 : 4]   => "GET /" => HTTP request header
/ tcpData[5 : 6]   => "s,"    => 's' is the ID for state machine and ',' is seperator
/ tcpData[7 : ...] => actual data we want to parse and store
/----------------------------------------------------------------------------*/
uint16_t parseTCP_setAndResponse(const char* name, uint8_t* eepromBaseAddr, uint8_t* ramBuffer, uint8_t maxLen,uint16_t tcpLen,uint8_t* respBuff)
{
  uint8_t ind = 0;

  while((tcpData[ind+7] != ' ') && (ind < (maxLen -1)))
  {
    ramBuffer[ind] = tcpData[ind+7];
    ind++;        
  }

  tcpLen = fill_tcp_data(respBuff,0,HTTP_RESPONSE_HEADER);

  if(ind < (maxLen -1))
  {
    ramBuffer[ind] = '\0';

    eeprom_update_block(ramBuffer,eepromBaseAddr,maxLen);

    tcpLen = fill_tcp_data(respBuff,tcpLen,name); 
    tcpLen = fill_tcp_data(respBuff,tcpLen," set to: ");
    tcpLen = fill_tcp_data(respBuff,tcpLen,ramBuffer);
    tcpLen = fill_tcp_data(respBuff,tcpLen,"\r\n");  
  }
  else
  {
    tcpLen = fill_tcp_data(respBuff,tcpLen,name);
    tcpLen = fill_tcp_data(respBuff,tcpLen," too long!\r\n");
  }

  return tcpLen;
}
/*---------------------------------------------------------------------------*/
uint16_t create_GetRequest(const uint8_t* urlBase, uint8_t* urlSuffix, uint8_t* respBuff, uint16_t tcpLen)
{
  tcpLen = fill_tcp_data(respBuff,tcpLen,"GET "); 
  tcpLen = fill_tcp_data(respBuff,tcpLen,urlSuffix); 
  tcpLen = fill_tcp_data(respBuff,tcpLen," HTTP/1.1\r\nUser-Agent: curl/7.37.1\r\nHost: "); 
  tcpLen = fill_tcp_data(respBuff,tcpLen,urlBase); 
  tcpLen = fill_tcp_data(respBuff,tcpLen,"\r\nAccept: */*\r\n\r\n"); 
  
  return tcpLen;
}
/*---------------------------------------------------------------------------*/
void fail_if_not_ok()
{
  if(check_ok() != 0)
  {
    /* Apply complete software reset */
    CCP = CCP_IOREG_gc;
    RST.CTRL = RST_SWRST_bm;
  }
}
/*---------------------------------------------------------------------------*/
void wait_until_gotIP()
{
  uint8_t cnt = 0;

  /* Wait until system get IP */
  do
  {
    cnt++;
    _delay_ms(100);     
    xprintf("AT+CIFSR\r\n");    
  }
  while((check_error() == 0) && (cnt < 50));
  
  /* Timeout! */
  if(cnt == 50)
  {
    /* Apply complete software reset */
    CCP = CCP_IOREG_gc;
    RST.CTRL = RST_SWRST_bm;
  }
}
/*---------------------------------------------------------------------------*/
