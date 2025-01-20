//*****************************************************************************
//
// circular_buffer.c - Used to store logs
//
//*****************************************************************************

#include <string.h>
#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"
#include "input_task.h"
#include "display_task.h"
#include "log_task.h"
#include "logProcessor_task.h"
#include "circular_buffer.h"

//*****************************************************************************

extern SemaphoreHandle_t LogSemaphore;

//*****************************************************************************

CircularBuffer buffer;                         // Instantiate a circular buffer

void init_buffer() {                           // Initialize the circular buffer

    buffer.head = 0;
    buffer.tail = 0;
}

// write a log entry to the buffer

uint32_t writeLog(const LogEntry* entry) {

    if (xSemaphoreTake(LogSemaphore, portMAX_DELAY)) {
        LogEntry* target = &buffer.entries[buffer.head];
        *target = *entry;

        buffer.head = (buffer.head + 1) % BUFFER_SIZE;
        if (buffer.head == buffer.tail) {
            buffer.tail = (buffer.tail + 1) % BUFFER_SIZE;
        }
        xSemaphoreGive(LogSemaphore);
        return 1;   // success
    }
    return 0;
}


uint32_t readLog(LogEntry* entry) {

    if (xSemaphoreTake(LogSemaphore, portMAX_DELAY)) {

        if (buffer.head == buffer.tail) {
            xSemaphoreGive(LogSemaphore);
            return 0;   // Buffer is empty
        }

        *entry = buffer.entries[buffer.tail];
        buffer.tail = (buffer.tail + 1) % BUFFER_SIZE;

        xSemaphoreGive(LogSemaphore);
        return 1;   // Successfully read a log
    }
    return 2;       // Failed to acquire mutex
}

//*****************************************************************************









