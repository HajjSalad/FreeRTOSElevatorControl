//*****************************************************************************

// input_task.c - Monitor the buttons and allow for user input floor requests

//*****************************************************************************

#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_memmap.h"
#include "inc/hw_gpio.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/rom.h"
#include "drivers/buttons.h"
#include "drivers/rgb.h"
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

// The stack size for the Input task.

#define INPUTTASKSTACKSIZE        128           // Stack size in words

TaskHandle_t xInputTaskHandle = NULL;           // InputTask Notification
extern TaskHandle_t xLogTaskHandle;
extern TaskHandle_t xLogProcessorTaskHandle;

extern QueueHandle_t DisplayQueue;

extern SemaphoreHandle_t UARTSemaphore;
extern SemaphoreHandle_t LogSemaphore;

uint8_t g_ui8SW1PressCount = 0;                 // Tracks SW1 presses
bool g_bSW2Pressed = false;                     // Tracks SW2 press

// [G, R, B] range is 0 to 0xFFFF per color.
static uint32_t g_pui32Colors[3] = { 0x0000, 0x0000, 0x0000 };

//*****************************************************************************

// Allows for sequential user input

uint8_t getSequenceInput(void) {
    uint8_t ui8RawState, ui8Delta;
    uint8_t ui8CurrentState = ButtonsPoll(&ui8Delta, &ui8RawState);

    // Handle SW1 press counts
    if (BUTTON_PRESSED(LEFT_BUTTON, ui8CurrentState, ui8Delta)) {
        g_ui8SW1PressCount++;
    }

    // Handle SW2 confirmation
    if (BUTTON_PRESSED(RIGHT_BUTTON, ui8CurrentState, ui8Delta)) {
        g_bSW2Pressed = true;
    }

    // Confirm input if SW2 is pressed
    if (g_bSW2Pressed) {
        uint8_t input = 0;

        if (g_ui8SW1PressCount == 1) {
            input = 1; // Input 1
        } else if (g_ui8SW1PressCount == 2) {
            input = 2; // Input 2
        } else if (g_ui8SW1PressCount == 3) {
            input = 3; // Input 3
        } else if (g_ui8SW1PressCount == 4) {
            input = 4; // Input 4
        } else if (g_ui8SW1PressCount == 5) {
            input = 5; // Input 5 for log display
        }

        // Reset state for next input sequence
        g_ui8SW1PressCount = 0;
        g_bSW2Pressed = false;

        return input;
    }

    return 0; // No input confirmed yet
}

//*****************************************************************************

// The Input Task

static void InputTask(void *pvParameters) {

    uint8_t ui8floorInput = 0;
    uint8_t toggle = 0;
    floorInput input;

    logMessage("Program starting..");

    while(1) {

        xSemaphoreTake(UARTSemaphore, portMAX_DELAY);
        UARTprintf("Enter %s floor: ", toggle == 0 ? "current" : "target");
        xSemaphoreGive(UARTSemaphore);
        logMessage("[InputTask] - Requesting user input..");

        while(1) {

            // Get the user input
            ui8floorInput = getSequenceInput();

            if (ui8floorInput != 0) {
                xSemaphoreTake(UARTSemaphore, portMAX_DELAY);
                UARTprintf("%d\n", ui8floorInput);
                xSemaphoreGive(UARTSemaphore);
                break;
            }
        }

        // Place user input into the struct
        input.floor = ui8floorInput;
        input.toggle = toggle;
        logMessage("[InputTask] - User input received.");

        if (ui8floorInput == 5) {
            // Give signal to LogProcessor to display the logs
            xTaskNotifyGive(xLogProcessorTaskHandle);
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        // Toggle between current and target floor
        toggle = !toggle;

        // Send the struct to DisplayTask
        if(xQueueSend(DisplayQueue, &input, portMAX_DELAY) == pdPASS) {

            logMessage("[InputTask] - Input sent to DisplayTask.");

            // Wait for DisplayTask to notify it has processed
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        } else {
            UARTprintf("\nQueue full. This should never happen.\n");
            while(1){}
        }

        // Delay here to allow lower priority tasks to run
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

//*****************************************************************************

// Initialize the input task.

uint32_t InputTaskInit(void) {

    static uint8_t ui8ColorsIndx;

    // Initialize the GPIOs and Timers that drive the LEDs.
    //
    RGBInit(1);
    RGBIntensitySet(0.3f);

    // Turn on Green LED
    // Indicates beginning of the program - Input task highest priority
    //
    ui8ColorsIndx = 1;
    g_pui32Colors[ui8ColorsIndx] = 0x8000;
    RGBColorSet(g_pui32Colors);


    // Unlock the GPIO LOCK register for Right button to work.
    //
    HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
    HWREG(GPIO_PORTF_BASE + GPIO_O_CR) = 0xFF;

    // Initialize the buttons
    //
    ButtonsInit();

    // Create the input task.
    //
    if(xTaskCreate(InputTask, (const portCHAR *)"Input",
                   INPUTTASKSTACKSIZE, NULL, tskIDLE_PRIORITY +
                   PRIORITY_INPUT_TASK, &xInputTaskHandle) != pdTRUE) {
        return(1);
    }

    return(0);
}
