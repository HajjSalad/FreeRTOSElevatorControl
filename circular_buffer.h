//*****************************************************************************
//
//  circular_buffer.h   -   header files for circular_buffer.c
//
//*****************************************************************************

#ifndef __CIRCULAR_BUFFER_H__
#define __CIRCULAR_BUFFER_H__

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#define BUFFER_SIZE     30       // Size of the circular buffer
#define MESSAGE_SIZE    256      // Size of message

//*****************************************************************************

typedef struct {                // LogEntry Structure
    uint32_t timestamp;         // timestamp of the log
    char message[MESSAGE_SIZE];              // log message
} LogEntry;

typedef struct {                            // CircularBuffer structure
    LogEntry entries[BUFFER_SIZE];          //
    uint32_t head;                          // pointer to head
    uint32_t tail;                          // pointer to tail
} CircularBuffer;

//*****************************************************************************
//
// Prototypes for the Circular buffer
//
//*****************************************************************************

extern void init_buffer();
extern uint32_t writeLog(const LogEntry* entry);
extern uint32_t readLog(LogEntry* entry);

//*****************************************************************************

#endif  // __CIRCULAR_BUFFER_H__




