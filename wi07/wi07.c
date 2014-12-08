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
				_delay_us(250);
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
				_delay_us(250);
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
			_delay_us(250);
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
			_delay_us(250);
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
	while(RingBuffer_GetCount(&Buffer))
	{
		RingBuffer_Remove(&Buffer);
	}

	xprintf("AT+CIPSEND=0,%d\r\n",len);

	wait_for_message("> ",10000);

	_delay_us(100);

	xprintf("%s",msg);

	while(check_ok() != 0);			
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
/*---------------------------------------------------------------------------*/