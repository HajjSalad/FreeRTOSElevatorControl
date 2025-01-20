//*****************************************************************************
//
// logProcessor_task.c - display logs to terminal via UART
//
//*****************************************************************************

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
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

// The stack size for the LogProcessor task.

#define LOGPROCESSORTASKSTACKSIZE        128                 // Stack size in words

//*****************************************************************************

TaskHandle_t xLogProcessorTaskHandle = NULL;   // LogProcessorTask Notification
extern TaskHandle_t xLogTaskHandle;

extern SemaphoreHandle_t LogSemaphore;
extern SemaphoreHandle_t UARTSemaphore;

//*****************************************************************************

// The LogProcessor Task

static void LogProcessorTask(void *pvParameters) {

    LogEntry entry;             // Instantiate a log entry

    while(1) {

        // Wait for signal to start reading logs
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        xSemaphoreTake(UARTSemaphore, portMAX_DELAY);
        UARTprintf("\n\t\t--- LOGS START ---\n");
        xSemaphoreGive(UARTSemaphore);

        while(readLog(&entry)) {
            // Display the log entry
            xSemaphoreTake(UARTSemaphore, portMAX_DELAY);
            UARTprintf("Timestamp: %5u ms\tMessage: %s\n", entry.timestamp, entry.message);
            xSemaphoreGive(UARTSemaphore);
        }

        xSemaphoreTake(UARTSemaphore, portMAX_DELAY);
        UARTprintf("\n\t\t--- LOGS END ---\n\n");
        xSemaphoreGive(UARTSemaphore);

        vTaskDelay(pdMS_TO_TICKS(10));
    }

}

//*****************************************************************************

// Initialize the LogProcessor Task

uint32_t LogProcessorTaskInit(void) {

    // Create the log task
    if(xTaskCreate(LogProcessorTask, (const portCHAR *)"LogProcessor", LOGPROCESSORTASKSTACKSIZE, NULL,
                   tskIDLE_PRIORITY + PRIORITY_LOGPROCESSOR_TASK, &xLogProcessorTaskHandle) != pdTRUE) {
        return(1);
    }

    return(0);
}
