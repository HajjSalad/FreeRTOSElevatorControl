//*****************************************************************************
//
// input_task.h - Prototypes for the Input task.
//
//*****************************************************************************

#ifndef __INPUT_TASK_H__
#define __INPUT_TASK_H__

#include <stdint.h>

typedef struct {
    uint8_t floor;          // floor value
    uint8_t toggle;           // 0 = currentFloor  1 = targetFloor
} floorInput;

//*****************************************************************************
//
// Prototypes for the input task.
//
//*****************************************************************************
extern uint32_t InputTaskInit(void);
extern uint8_t getSequenceInput(void);

#endif // __INPUT_TASK_H__
