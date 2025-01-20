//*****************************************************************************

// display_task.c - Displays the elevator position and movements

//*****************************************************************************

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
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

// The stack size for the Display task.

#define DISPLAYTASKSTACKSIZE        128                 // Stack size in words

// The item size and queue size for the Display message queue.

#define DISPLAY_ITEM_SIZE           sizeof(floorInput)
#define DISPLAY_QUEUE_SIZE          5

//*****************************************************************************

QueueHandle_t DisplayQueue;                   // Display Queue

extern TaskHandle_t xInputTaskHandle;
extern TaskHandle_t xLogTaskHandle;

extern SemaphoreHandle_t UARTSemaphore;
extern SemaphoreHandle_t LogSemaphore;

//*****************************************************************************

uint8_t ui8lastCall = 1;

// Display status of elevetor ie Floor 1 [ ], Floor 2 [ ]
void elevator_status(uint8_t ui8currentCall) {
    int i;
    for (i=4; i > 0; i--) {
        if(i == ui8currentCall) {
            xSemaphoreTake(UARTSemaphore, portMAX_DELAY);
            UARTprintf("Floor %d [E] - Elevator is here\n", i);
            xSemaphoreGive(UARTSemaphore);
        } else {
            xSemaphoreTake(UARTSemaphore, portMAX_DELAY);
            UARTprintf("Floor %d [ ]\n", i);
            xSemaphoreGive(UARTSemaphore);
        }
    }
}

// Display motion of elevator ie Elevator at floor 4..
void elevator_motion(uint8_t ui8previousCall, uint8_t ui8currentCall) {

    if (ui8previousCall == ui8currentCall) {
        xSemaphoreTake(UARTSemaphore, portMAX_DELAY);
        UARTprintf("\nYou are at floor %d.\n\n", ui8previousCall);
        xSemaphoreGive(UARTSemaphore);
    }
    // Elevator going up
    else if (ui8previousCall < ui8currentCall) {
        xSemaphoreTake(UARTSemaphore, portMAX_DELAY);
        UARTprintf("\nElevator going up..\n");

        int i;
        for (i = ui8previousCall + 1; i < ui8currentCall; i++){
            UARTprintf("Elevator at floor %d..\n", i);
        }
        UARTprintf("Elevator arrived at floor %d\n\n", ui8currentCall);
        xSemaphoreGive(UARTSemaphore);
    }
    else {
        xSemaphoreTake(UARTSemaphore, portMAX_DELAY);
        UARTprintf("\nElevator going down..\n");

        int i;
        for (i = ui8previousCall - 1; i > ui8currentCall; i--){
            UARTprintf("Elevator at floor %d..\n", i);
        }
        UARTprintf("Elevator arrived at floor %d\n\n", ui8currentCall);
        xSemaphoreGive(UARTSemaphore);
    }
}

//*****************************************************************************

// The Display task

static void DisplayTask(void *pvParameters) {

    uint8_t *pui8lastCall = (uint8_t*)pvParameters;

    floorInput receivedInput;
    uint8_t ui8currentFloor, ui8previousCall, ui8targetFloor;

    while(1) {

        // Wait for input from InputTask
        if(xQueueReceive(DisplayQueue, &receivedInput, 0) == pdPASS) {

            logMessage("[DisplayTask] - Input recieved from InputTask.");
            logMessage("[DisplayTask] - Elevator service starting..");

            if (receivedInput.toggle == 0) {                                // current floor request
                ui8currentFloor = receivedInput.floor;
                ui8previousCall = *pui8lastCall;                            // Copy the last call - might be changed
                elevator_motion(ui8previousCall, ui8currentFloor);
                *pui8lastCall = ui8currentFloor;                            // Update the last floor visited by the elevator
                elevator_status(ui8currentFloor);                           // Graphical representation of the elevator
            }
            else  {                                                         // target floor request
                ui8targetFloor = receivedInput.floor;
                ui8previousCall = *pui8lastCall;                            // Copy the last call - where the elevator was left
                elevator_motion(ui8previousCall, ui8targetFloor);
                *pui8lastCall = ui8targetFloor;                            // Update the last floor visited by the elevator
                elevator_status(ui8targetFloor);                           // Graphical representation of the elevator
            }
            logMessage("[DisplayTask] - Elevator service complete.");
            // Notify InputTask that processing is complete
            xTaskNotifyGive(xInputTaskHandle);
        }
        // Delay here to allow lower priority tasks to run
        vTaskDelay(pdMS_TO_TICKS(50));                      // Delay for 50ms
    }
}

//*****************************************************************************

// Initialize the Display task.

uint32_t DisplayTaskInit(void) {

    // Create a queue for sending messages to the Display task.
    DisplayQueue = xQueueCreate(DISPLAY_QUEUE_SIZE, DISPLAY_ITEM_SIZE);
    if (DisplayQueue == NULL) {
        UARTprintf("DisplayQueue creation failed\n");
    }

    // Create the Display task.
    if(xTaskCreate(DisplayTask, (const portCHAR *)"Display", DISPLAYTASKSTACKSIZE, (void*)&ui8lastCall,
                   tskIDLE_PRIORITY + PRIORITY_DISPLAY_TASK, NULL) != pdTRUE) {
        return(1);
    }

    return(0);
}
