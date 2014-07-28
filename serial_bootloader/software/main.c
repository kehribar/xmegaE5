/*-------------------------------------------------------------------------------------------------
/ Computer side software for XmegaE5 UART bootloader.
/--------------------------------------------------------------------------------------------------
/ TODO:
/   - A lot ...
/--------------------------------------------------------------------------------------------------
/ <ihsan@kehribar.me> - August 2014
/------------------------------------------------------------------------------------------------*/
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include "serial_lib.h"
/*-----------------------------------------------------------------------------------------------*/
uint8_t dataBuffer[65536];
/*-----------------------------------------------------------------------------------------------*/
#define PAGE_SIZE 128
#define SERIAL_PATH "/dev/ttyUSB0"
/*-----------------------------------------------------------------------------------------------*/
static int parseIntelHex(char *hexfile, uint8_t* buffer, int *startAddr, int *endAddr);
static int parseHex(FILE *file_pointer, int num_digits);
static int parseUntilColon(FILE *file_pointer);
/*-----------------------------------------------------------------------------------------------*/
/* Taken from: http://www.linuxquestions.org/questions/programming-9/manually-controlling-rts-cts-326590/#post1658463 */
int setRTS(int fd, int level)
{
    int status;

    if (ioctl(fd, TIOCMGET, &status) == -1) {
        perror("setRTS(): TIOCMGET");
        return 0;
    }
    if (level)
        status |= TIOCM_RTS;
    else
        status &= ~TIOCM_RTS;
    if (ioctl(fd, TIOCMSET, &status) == -1) {
        perror("setRTS(): TIOCMSET");
        return 0;
    }
    return 1;
}
/*-----------------------------------------------------------------------------------------------*/
int connectDevice(char* path)
{
    int fd = -1;    

    fd = serialport_init(SERIAL_PATH,115200,'n');

    if(fd < 0)
    {
        printf("[err]: Connection error.\n");
        return -1;
    }
    else
    {
        printf("[dbg]: Conection OK.\n");
        return fd;
    }       
}
/*-----------------------------------------------------------------------------------------------*/
int getVersion(fd)
{
    char msg;

    serialport_writebyte(fd,'v');

    /* Read the response */
    if(readRawBytes(fd,&msg,1,10000) < 0)
    {
        /* Timeout or read problem */
        return -1;
    }

    return msg;
}
/*-----------------------------------------------------------------------------------------------*/
int readACK(fd)
{
    char msg;

    /* Read the response */
    if(readRawBytes(fd,&msg,1,10000) < 0)
    {
        /* Timeout or read problem */
        return -1;
    }

    if(msg == 'Y')
    {
        /* Return OK */
        return 1;
    }
    else
    {
        /* Wrong response ... */
        return -1;
    }
}
/*-----------------------------------------------------------------------------------------------*/
int sendPing(int fd)
{    
    /* Send ping message */
    serialport_writebyte(fd,'a');    

    return readACK(fd);
}
/*-----------------------------------------------------------------------------------------------*/
int main(int argc, char *argv[]) 
{    
    int i;
    int fd;
    int offset;
    uint8_t t8;
    int pageNumber;
    int endAddress = 0;
    int startAddress = 1;
    char* fileName = NULL;

    if(argc == 1)
    {
        printf("[err]: What is the file name?\n");        
        return 0;
    }
    else if(argc > 2)
    {
        printf("[err]: Unneccessary parameters!\n");
        return 0;
    }

    fd = connectDevice(SERIAL_PATH);

    if(fd < 0)
    {
        return 0;
    }

    setRTS(fd,1);
    setRTS(fd,0);
    setRTS(fd,1);

    if(sendPing(fd) > 0)
    {
        printf("[dbg]: Ping OK\n");
    }
    else
    {
        printf("[err]: Ping problem\n");
        return 0;
    }

    printf("[dbg]: Firmware version: %d\n",getVersion(fd));

    fileName = argv[1];

    memset(dataBuffer, 0xFF, sizeof(dataBuffer));

    parseIntelHex(fileName, dataBuffer, &startAddress, &endAddress);

    if(startAddress != 0)
    {
        printf("[err]: You should change the startAddress = 0 assumption\n");
        return 0;
    }

    if(endAddress > (32768))
    {
        printf("[err]: Program size is too big!\n");
	return 0;
    }

    printf("[dbg]: Erasing the memory ...\n");
    serialport_writebyte(fd,'d');
    if (readACK(fd) > 0)
    {
        printf("[dbg]: ACK OK\n");
    }
    else
    {
        printf("[dbg]: ACK problem\n");
        return 0;
    }

    i = 0;    
    offset = 0;
    pageNumber = 0;
    while(offset<endAddress)
    {        
        printf("\n");
        printf("[dbg]: Page number: %d\n",pageNumber);
        printf("[dbg]: Page base address: %d\n",offset);
        
        /* Fill the page buffer command */
        serialport_writebyte(fd,'b');

        if (readACK(fd) > 0)
        {
            printf("[dbg]: ACK OK\n");
        }
        else
        {
            printf("[dbg]: ACK problem\n");
            return 0;
        }
                
        printf("    ");
        for(i=0;i<PAGE_SIZE;i++)
        {                        
            printf("[%3d] %2X   ",i,dataBuffer[i+offset]);     
            if((i%8)==7)
            {
                printf("\n    ");
            }                   
            serialport_writebyte(fd,dataBuffer[i+offset]);
        }
        printf("\n");

        /* Write the page command */
        serialport_writebyte(fd,'c');  

        if (readACK(fd) > 0)
        {
            printf("[dbg]: ACK OK\n");
        }
        else
        {
            printf("[dbg]: ACK problem\n");
            return 0;
        }

        t8 = (offset >> 0) & 0xFF;
        serialport_writebyte(fd,t8);

        t8 = (offset >> 8) & 0xFF;
        serialport_writebyte(fd,t8);

        t8 = (offset >> 16) & 0xFF;
        serialport_writebyte(fd,t8);

        t8 = (offset >> 24) & 0xFF;
        serialport_writebyte(fd,t8);

        if (readACK(fd) > 0)
        {
            printf("[dbg]: ACK OK\n");
        }
        else
        {
            printf("[dbg]: ACK problem\n");
            return 0;
        }

        offset += PAGE_SIZE;
        pageNumber++;
    }
    
    /* Jump to the user app */
    serialport_writebyte(fd,'x');

    serialport_close(fd);
    return 0;
}
/*-----------------------------------------------------------------------------------------------*/
static int parseIntelHex(char *hexfile, uint8_t* buffer, int *startAddr, int *endAddr) 
{
  int address, base, d, segment, i, lineLen, sum;
  FILE *input;
  
  input = strcmp(hexfile, "-") == 0 ? stdin : fopen(hexfile, "r");
  if (input == NULL) {
    printf("> Error opening %s: %s\n", hexfile, strerror(errno));
    return 1;
  }
  
  while (parseUntilColon(input) == ':') {
    sum = 0;
    sum += lineLen = parseHex(input, 2);
    base = address = parseHex(input, 4);
    sum += address >> 8;
    sum += address;
    sum += segment = parseHex(input, 2);  /* segment value? */
    if (segment != 0) {   /* ignore lines where this byte is not 0 */
      continue;
    }
    
    for (i = 0; i < lineLen; i++) {
      d = parseHex(input, 2);
      buffer[address++] = d;
      sum += d;
    }
    
    sum += parseHex(input, 2);
    if ((sum & 0xff) != 0) {
      printf("> Warning: Checksum error between address 0x%x and 0x%x\n", base, address);
    }
    
    if(*startAddr > base) {
      *startAddr = base;
    }
    if(*endAddr < address) {
      *endAddr = address;
    }
  }
  
  fclose(input);
  return 0;
}
/*-----------------------------------------------------------------------------------------------*/
static int parseUntilColon(FILE *file_pointer) 
{
  int character;
  
  do {
    character = getc(file_pointer);
  } while(character != ':' && character != EOF);
  
  return character;
}
/*-----------------------------------------------------------------------------------------------*/
static int parseHex(FILE *file_pointer, int num_digits) 
{
  int iter;
  char temp[9];

  for(iter = 0; iter < num_digits; iter++) {
    temp[iter] = getc(file_pointer);
  }
  temp[iter] = 0;
  
  return strtol(temp, NULL, 16);
}
/*-----------------------------------------------------------------------------------------------*/
