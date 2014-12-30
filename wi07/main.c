/*-----------------------------------------------------------------------------
/
/
/----------------------------------------------------------------------------*/
#include "main.h"
/*---------------------------------------------------------------------------*/
void init_adc();
void init_uart();
void client_mode();
void server_mode();
void dbg(uint8_t ch);
void init_hardware();
void fail_if_not_ok();
void initClock_32Mhz();
uint16_t read_adc(uint8_t ch);
void sendch_module(uint8_t ch);
/*---------------------------------------------------------------------------*/
int main()
{  
  init_hardware();    
    
  set_rgb(255,0,0); 

  /* Wait for the powerup */
  wait_for_message("com]\r\n",10000);

  /* Send data to PC */
  xfprintf(dbg,"Hello World!\r\n");

  /* Enter AP mode for configuration */
  if(digitalRead(D,3) == LOW)
  {
    server_mode();
  }
  else
  {
    client_mode();
  } 

  return 0;
}
/*---------------------------------------------------------------------------*/
void client_mode()
{  
  uint8_t i = 1;
  int16_t integer;
  int16_t decimal;
  int32_t temp = 0;
  uint16_t tcp_len;
  
  /* Read the settings */
  eeprom_read_block(SSID_name,SSID_name_addr,32);
  eeprom_read_block(SSID_pass,SSID_pass_addr,32);
  eeprom_read_block(PublicKey,PublicKey_addr,32);
  eeprom_read_block(PrivateKey,PrivateKey_addr,32);

  xprintf("AT+CWMODE=1\r\n");
  _delay_ms(100);

  xprintf("AT+RST\r\n");
  fail_if_not_ok();

  /* Wait for the powerup */
  wait_for_message("com]\r\n",10000);

  /* SSID details ... */
  xprintf("AT+CWJAP=\"");
  xprintf("%s\",\"",SSID_name);
  xprintf("%s\"\r\n",SSID_pass);

  /* Check the result of join */
  fail_if_not_ok();

  /* Wait until system get IP */
  wait_until_gotIP();

  /* ... */
  xprintf("AT+CIPMUX=1\r\n");
  fail_if_not_ok();

  while(filterStable == 0)
  {
    /*-------------------------------------------------------------------*/
    /* In place average */
    /*-------------------------------------------------------------------*/
    while(i++)
    {
      temp += read_adc(1);
    }
    temp >>= 8;
    temp -= 500; /* temperature sensor have some offset for 0 celcius */
    /*-------------------------------------------------------------------*/

    /*-------------------------------------------------------------------*/ 
    /* Moving average */
    /*-------------------------------------------------------------------*/   
    totalSum = totalSum + temp - movingBuffer[movingIndex];
    movingBuffer[movingIndex] = temp;
    movingIndex++;
    if(movingIndex == 8)
    {     
      movingIndex = 0;
      filterStable = 1;
    }
    /*-------------------------------------------------------------------*/
  }

  while(1)
  {
    set_rgb(255,255,255);

    /*-------------------------------------------------------------------*/
    /* In place average */
    /*-------------------------------------------------------------------*/
    while(i++)
    {
      temp += read_adc(1);
    }
    temp >>= 8;
    temp -= 500; /* temperature sensor have some offset for 0 celcius */
    /*-------------------------------------------------------------------*/

    /*-------------------------------------------------------------------*/ 
    /* Moving average */
    /*-------------------------------------------------------------------*/   
    totalSum = totalSum + temp - movingBuffer[movingIndex];
    movingBuffer[movingIndex] = temp;
    movingIndex++;
    if(movingIndex == 8)
    {     
      movingIndex = 0;
    }
    temp = totalSum >> 3;
    /*-------------------------------------------------------------------*/

    integer = temp / 10;
    decimal = temp - (10 * integer);    

    /* Open a socket to "data.sparkfun.com" address at port 80 */
    xprintf("AT+CIPSTART=0,\"TCP\",\"data.sparkfun.com\",80\r\n");
    fail_if_not_ok();

    wait_for_message("Linked\r\n",10000); 

    xsprintf(smallBuffer,"/input/%s?private_key=%s&temp=%d.%d",PublicKey,PrivateKey,integer,decimal);
    tcp_len = create_GetRequest("data.sparkfun.com", smallBuffer, txBuffer, 0);

    send_TCPData(txBuffer,tcp_len);

    /* Wait for some time for if we get some response */
    _delay_ms(500);

    /* close the socket */
    xprintf("AT+CIPCLOSE=0\r\n");
    fail_if_not_ok();

    wait_for_message("Unlink\r\n",10000);

    /* Wait some time for next data */
    set_rgb(0,255,0);
    _delay_ms(40000);
  }
}
/*---------------------------------------------------------------------------*/
void server_mode()
{  
  uint16_t tcp_len;

  xprintf("AT+CWMODE=2\r\n");
  _delay_ms(100);

  xprintf("AT+RST\r\n");
  check_ok();

  /* Wait for the powerup */
  wait_for_message("com]\r\n",10000);

  /* SSID details ... */
  xprintf("AT+CWSAP=\"myssid\",\"123456789\",1,0\r\n");

  /* Check the result of join */
  fail_if_not_ok();

  /* Wait until system get IP */
  wait_until_gotIP();

  /* ... */
  xprintf("AT+CIPMUX=1\r\n");
  fail_if_not_ok();
  
  /* Enable server at port 80 */
  xprintf("AT+CIPSERVER=1,80\r\n");
  fail_if_not_ok(); 

  set_rgb(0,0,255);

  while(1)
  {    
    waitTCPMessage_blocking();

    /*-----------------------------------------------------------------
    / Respond to HTTP GET messages - Basic web server
    /------------------------------------------------------------------
    / Example of a starting part of an HTTP GET message is: 
    /   GET /index.html HTTP/1.1
    /----------------------------------------------------------------*/
    if(strncmp("GET /",tcpData,5) == 0)
    {
      memset(txBuffer,0x00,sizeof(txBuffer));    

      switch(tcpData[5])
      {
        /* Default web page */
        case ' ':
        {
          tcp_len = fill_tcp_data(txBuffer,0,HTTP_RESPONSE_HEADER);
          tcp_len = fill_tcp_data(txBuffer,tcp_len,"OK!\r\n");               
          break;
        }
        /* SSID name */
        case 's':
        {
          tcp_len = parseTCP_setAndResponse("SSID name", SSID_name_addr, SSID_name, sizeof(SSID_name), 0, txBuffer);
          break;
        } 
        /* SSID  password */
        case 'p':
        {
          tcp_len = parseTCP_setAndResponse("SSID pass", SSID_pass_addr, SSID_pass, sizeof(SSID_pass), 0, txBuffer);
          break;
        } 
        /* Set data.sparkfun.com Public Key */
        case 'x':
        {
          tcp_len = parseTCP_setAndResponse("PublicKey", PublicKey_addr, PublicKey, sizeof(PublicKey), 0, txBuffer);
          break;
        }
        /* Set data.sparkfun.com Private Key */
        case 'y':
        {
          tcp_len = parseTCP_setAndResponse("PrivateKey", PrivateKey_addr, PrivateKey, sizeof(PrivateKey), 0, txBuffer);
          break;
        }
        /* Unknown command */
        default:
        {              
          tcp_len = fill_tcp_data(txBuffer,0,HTTP_RESPONSE_HEADER);
          tcp_len = fill_tcp_data(txBuffer,tcp_len,"Unknown command!\r\n");    
          break;
        }
      } 

      /* send the response */
      send_TCPData(txBuffer,tcp_len);

      /* close the socket */
      xprintf("AT+CIPCLOSE=0\r\n");
      fail_if_not_ok();
    }
  }
}
/*---------------------------------------------------------------------------*/
