//*****************************************************************************
//
// main.c - Entry point and main flow of the program
//
//*****************************************************************************

#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "utils/uartstdio.h"
#include "input_task.h"
#include "display_task.h"
#include "log_task.h"
#include "logProcessor_task.h"
#include "circular_buffer.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

//*****************************************************************************

// mutex to protect concurrent access of UART from multiple tasks.

SemaphoreHandle_t UARTSemaphore;     // Semaphore for UART synchronization
SemaphoreHandle_t LogSemaphore;      // Semaphore for Log synchronization

extern TaskHandle_t xLogTaskHandle;

//*****************************************************************************

// This hook is called by FreeRTOS when an stack overflow error is detected.

void vApplicationStackOverflowHook(xTaskHandle *pxTask, char *pcTaskName) {
    while(1) {}
}

//*****************************************************************************

// Configure the UART and its pins.

void ConfigureUART(void) {

    // Enable the GPIO Peripheral used by UART0.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    // Enable UART0
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);

    // Configure GPIO Pins for UART0
    //
    ROM_GPIOPinConfigure(GPIO_PA0_U0RX);
    ROM_GPIOPinConfigure(GPIO_PA1_U0TX);
    ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    // Use the internal 16MHz oscillator as the UART0 clock source.
    //
    UARTClockSourceSet(UART0_BASE, UART_CLOCK_PIOSC);

    // Initialize the UART0 for console I/O.
    //
    UARTStdioConfig(0, 115200, 16000000);

}

//*****************************************************************************

// Initialize FreeRTOS and start the initial set of tasks.

int main(void) {

    // Set the clocking to run at 50 MHz from the PLL.
    //
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ |
                       SYSCTL_OSC_MAIN);

    // Initialize the UART and configure it for 115,200, 8-N-1 operation.
    //
    ConfigureUART();

    // Send notification to LogTask to wake up and start logs
    xTaskNotifyGive(xLogTaskHandle);

    // Print demo introduction.
    //
    UARTprintf("\nWelcome to FreeRTOS Elevator Control System Demo!\n\n");
    UARTprintf("Floor 4 [ ]\n");
    UARTprintf("Floor 3 [ ]\n");
    UARTprintf("Floor 2 [ ]\n");
    UARTprintf("Floor 1 [E] <- Elevator here\n\n");

    // Create a mutex to guard the UART and Log.
    //
    UARTSemaphore = xSemaphoreCreateMutex();
    LogSemaphore = xSemaphoreCreateMutex();

    // Create the Display task.
    //
    if(DisplayTaskInit() != 0) {
        while(1) {}
    }

    // Create the Input task.
    //
    if(InputTaskInit() != 0) {
        while(1) {}
    }

    if(LogTaskInit() != 0) {
        while(1) {}
    }

    if(LogProcessorTaskInit() != 0) {
        while(1) {}
    }

    // Start the scheduler.  This should not return.
    //
    vTaskStartScheduler();

    // In case the scheduler returns for some reason, print an error and loop forever
    //
    while(1) {}


}

//*****************************************************************************
