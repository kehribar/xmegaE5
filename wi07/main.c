/*-----------------------------------------------------------------------------
/
/
/----------------------------------------------------------------------------*/
#include "main.h"
/*---------------------------------------------------------------------------*/
void init_adc();
void init_uart();
void dbg(uint8_t ch);
void init_hardware();
void fail_if_not_ok();
void initClock_32Mhz();
uint16_t read_adc(uint8_t ch);
void sendch_module(uint8_t ch);
/*---------------------------------------------------------------------------*/
void client_mode();
void server_mode();
/*---------------------------------------------------------------------------*/
int main()
{
  init_hardware();  
    
  set_rgb(255,0,0); 

  /* Wait for the powerup */
  wait_for_message("com]\r\n",10000);

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
  int32_t temp;
  uint8_t i = 1;
  int16_t integer;
  int16_t decimal;
  
  /* Read the SSID settings */
  eeprom_read_block(SSID_name,SSID_name_addr,32);
  eeprom_read_block(SSID_pass,SSID_pass_addr,32);
  eeprom_read_block(PublicKey,PublicKey_addr,32);
  eeprom_read_block(PrivateKey,PrivateKey_addr,32);

  xprintf("AT+CWMODE=1\r\n");
  check_ok();

  xprintf("AT+RST\r\n");
  check_ok();

  /* Wait for the powerup */
  wait_for_message("com]\r\n",10000);

  /* SSID details ... */
  xprintf("AT+CWJAP=\"");
  xprintf("%s\",\"",SSID_name);
  xprintf("%s\"\r\n",SSID_pass);

  /* Check the result of join */
  fail_if_not_ok();

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

    xprintf("AT+CIPSTART=0,\"TCP\",\"data.sparkfun.com\",80\r\n");
    check_ok();

    wait_for_message("Linked\r\n",10000); 

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

    xsprintf(tmpBuffer,"GET /input/%s?private_key=%s&temp=%d.%d HTTP/1.1\r\nUser-Agent: curl/7.37.1\r\nHost: data.sparkfun.com\r\nAccept: */*\r\n\r\n",
      PublicKey,PrivateKey,integer,decimal);

    send_TCPData(tmpBuffer,strlen(tmpBuffer));

    wait_for_message("+IPD",50000);

    _delay_ms(100);

    xprintf("AT+CIPCLOSE=0\r\n");
    wait_for_message("Unlink\r\n",10000);

    set_rgb(0,255,0);
    _delay_ms(40000);
  }
}
/*---------------------------------------------------------------------------*/
void server_mode()
{
  uint16_t len;
  uint8_t ind = 0;

  xprintf("AT+CWMODE=2\r\n");
  check_ok();

  xprintf("AT+RST\r\n");
  check_ok();

  /* Wait for the powerup */
  wait_for_message("com]\r\n",10000);

  /* SSID details ... */
  xprintf("AT+CWSAP=\"myssid\",\"123456789\",1,0\r\n");

  /* Check the result of join */
  fail_if_not_ok();

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

  set_rgb(0,0,255);

  uint16_t tcp_len;

  while(1)
  {
    ind = 0;
    
    len = waitTCPMessage_blocking();

    memset(tmpBuffer,0x00,256);    
    tcp_len = fill_tcp_data(tmpBuffer,0,HTTP_RESPONSE_HEADER);

    switch(tcpData[5])
    {
      /* SSID name */
      case 's':
      {
        while(tcpData[ind+7] != ' ')
        {
          SSID_name[ind] = tcpData[ind+7];
          ind++;        
        }

        SSID_name[ind] = '\0';

        eeprom_update_block(SSID_name,SSID_name_addr,32);

        tcp_len = fill_tcp_data(tmpBuffer,tcp_len,"SSID name set to: ");
        tcp_len = fill_tcp_data(tmpBuffer,tcp_len,SSID_name);
        tcp_len = fill_tcp_data(tmpBuffer,tcp_len,"\r\n");        

        break;
      } 
      /* SSID  password */
      case 'p':
      {
        while(tcpData[ind+7] != ' ')
        {
          SSID_pass[ind] = tcpData[ind+7];
          ind++;        
        }

        SSID_pass[ind] = '\0';
        
        eeprom_update_block(SSID_pass,SSID_pass_addr,32);

        tcp_len = fill_tcp_data(tmpBuffer,tcp_len,"SSID pass set to: ");
        tcp_len = fill_tcp_data(tmpBuffer,tcp_len,SSID_pass);
        tcp_len = fill_tcp_data(tmpBuffer,tcp_len,"\r\n");  
        
        break;
      } 
      /* Set data.sparkfun.com Public Key */
      case 'x':
      {
        while(tcpData[ind+7] != ' ')
        {
          PublicKey[ind] = tcpData[ind+7];
          ind++;        
        }

        PublicKey[ind] = '\0';
        
        eeprom_update_block(PublicKey,PublicKey_addr,32);

        tcp_len = fill_tcp_data(tmpBuffer,tcp_len,"PublicKey set to: ");
        tcp_len = fill_tcp_data(tmpBuffer,tcp_len,PublicKey);
        tcp_len = fill_tcp_data(tmpBuffer,tcp_len,"\r\n");  
        
        break;
      }
      /* Set data.sparkfun.com Private Key */
      case 'y':
      {
        while(tcpData[ind+7] != ' ')
        {
          PrivateKey[ind] = tcpData[ind+2];
          ind++;        
        }

        PrivateKey[ind] = '\0';
        
        eeprom_update_block(PrivateKey,PrivateKey_addr,32);

        tcp_len = fill_tcp_data(tmpBuffer,tcp_len,"PrivateKey set to: ");
        tcp_len = fill_tcp_data(tmpBuffer,tcp_len,PrivateKey);
        tcp_len = fill_tcp_data(tmpBuffer,tcp_len,"\r\n");  

        break;
      }
      /* Unknown command */
      default:
      {        
    
        tcp_len = fill_tcp_data(tmpBuffer,tcp_len,"Unknown command!\r\n");    

        break;
      }
    } 

    send_TCPData(tmpBuffer,tcp_len);
    xprintf("AT+CIPCLOSE=0\r\n");

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
void sendch_module(uint8_t ch)
{
  while(!(USARTC0.STATUS & USART_DREIF_bm));
  USARTC0.DATA = ch;
}
/*---------------------------------------------------------------------------*/
void dbg(uint8_t ch)
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

  /* Set UART pin driver states */
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
  xdev_out(sendch_module);
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

  ADCA.CH0.AVGCTRL = ADC_SAMPNUM_8X_gc | (3 << 4);

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
void init_hardware()
{
  initClock_32Mhz();
  init_uart();
  init_adc();

  RingBuffer_InitBuffer(&Buffer, BufferData, sizeof(BufferData));

  /* Software PWM timer */   
  TCC4.CTRLA = TC45_CLKSEL_DIV256_gc; /* 32Mhz / 256 => 125 kHz */
  TCC4.PER = 9; /* 125 kHz / (9+1) => 12.5 kHz */
  TCC4.INTCTRLA = 0x01; /* Overflow interrupt level */

  /* Wifi module reset & control pins */
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
}
/*---------------------------------------------------------------------------*/
