//*****************************************************************************
//
// log_task.c - Store logs in a circular buffer
//
//*****************************************************************************

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "drivers/buttons.h"
#include "utils/uartstdio.h"
#include "input_task.h"
#include "display_task.h"
#include "log_task.h"
#include "logProcessor_task.h"
#include "circular_buffer.h"
#include "priorities.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

//*****************************************************************************

// The stack size for the Log task.

#define LOGTASKSTACKSIZE        128                 // Stack size in words

//*****************************************************************************

TaskHandle_t xLogTaskHandle = NULL;                 // LogTask Notification
extern TaskHandle_t xLogProcessorTaskHandle;

extern SemaphoreHandle_t LogSemaphore;
extern SemaphoreHandle_t UARTSemaphore;

//*****************************************************************************

// log message

void logMessage(const char* message) {

    LogEntry entry;         // Instantiate a log entry

    // Populate the log entry
    entry.timestamp = xTaskGetTickCount();
    strncpy(entry.message, message, sizeof(entry.message) - 1);
    entry.message[sizeof(entry.message) - 1] = '\0';

    // write to the circular buffer
    if (!writeLog(&entry)) {
        xSemaphoreTake(UARTSemaphore, portMAX_DELAY);
        UARTprintf("Warning: Not written to buffer");
        xSemaphoreGive(UARTSemaphore);
    }
}

//*****************************************************************************

// The Log Task

static void LogTask(void *pvParameters) {

    while(1) {

        // wait for notification from main.c to start log
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        vTaskDelay(pdMS_TO_TICKS(10));
    }

}

//*****************************************************************************

// Initialize the Log Task

uint32_t LogTaskInit(void) {

    // Initialize the buffer
    init_buffer();

    // Create the log task
    if(xTaskCreate(LogTask, (const portCHAR *)"Log", LOGTASKSTACKSIZE, NULL,
                   tskIDLE_PRIORITY + PRIORITY_LOG_TASK, &xLogTaskHandle) != pdTRUE) {
        return(1);
    }

    return(0);
}


