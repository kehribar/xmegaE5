/*******************************************************************************************************************
* Custom version of Ring Buffer implementation from LUFA library.
*******************************************************************************************************************/
#include <stdint.h>
#include <avr/interrupt.h>
/*----------------------------------------------------------------------------------------------------------------*/
typedef struct
{
    uint8_t* In; /**< Current storage location in the circular buffer. */
    uint8_t* Out; /**< Current retrieval location in the circular buffer. */
    uint8_t* Start; /**< Pointer to the start of the buffer's underlying storage array. */
    uint8_t* End; /**< Pointer to the end of the buffer's underlying storage array. */
    uint16_t Size; /**< Size of the buffer's underlying storage array. */
    uint16_t Count; /**< Number of bytes currently stored in the buffer. */
} RingBuffer_t;
/*----------------------------------------------------------------------------------------------------------------*/
static inline void RingBuffer_InitBuffer(RingBuffer_t* Buffer,uint8_t* const DataPtr,const uint16_t Size)
{    
    Buffer->In     = DataPtr;
    Buffer->Out    = DataPtr;
    Buffer->Start  = &DataPtr[0];
    Buffer->End    = &DataPtr[Size];
    Buffer->Size   = Size;
    Buffer->Count  = 0; 
}
/*----------------------------------------------------------------------------------------------------------------*/
static inline uint16_t RingBuffer_GetCount(RingBuffer_t* const Buffer)
{
    uint16_t Count;
    uint8_t temp = SREG;
    cli();

        Count = Buffer->Count;

    SREG = temp;
    return Count;
}
/*----------------------------------------------------------------------------------------------------------------*/
static inline uint16_t RingBuffer_GetFreeCount(RingBuffer_t* const Buffer)
{
    return (Buffer->Size - RingBuffer_GetCount(Buffer));
}
/*----------------------------------------------------------------------------------------------------------------*/
static inline uint8_t RingBuffer_IsEmpty(RingBuffer_t* const Buffer)
{
    return (RingBuffer_GetCount(Buffer) == 0);
}
/*----------------------------------------------------------------------------------------------------------------*/
static inline uint8_t RingBuffer_IsFull(RingBuffer_t* const Buffer)
{
    return (RingBuffer_GetCount(Buffer) == Buffer->Size);
}
/*----------------------------------------------------------------------------------------------------------------*/
static inline void RingBuffer_Insert(RingBuffer_t* Buffer, const uint8_t Data)
{    
    *Buffer->In = Data;

    if (++Buffer->In == Buffer->End)
      Buffer->In = Buffer->Start;

    uint8_t temp = SREG;
    cli();

        Buffer->Count++;

   SREG = temp;
}
/*----------------------------------------------------------------------------------------------------------------*/
static inline uint8_t RingBuffer_Remove(RingBuffer_t* Buffer)
{    
    uint8_t Data = *Buffer->Out;

    if (++Buffer->Out == Buffer->End)
      Buffer->Out = Buffer->Start;

    uint8_t temp = SREG;
    cli();

        Buffer->Count--;

    SREG = temp;

    return Data;
}
/*----------------------------------------------------------------------------------------------------------------*/
static inline uint8_t RingBuffer_Peek(RingBuffer_t* const Buffer)
{
    return *Buffer->Out;
}
/*----------------------------------------------------------------------------------------------------------------*/