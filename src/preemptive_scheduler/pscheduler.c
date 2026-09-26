/* INCLUDES ************************************************************/
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include "pscheduler.h"

/* DEFINES & MACROS ****************************************************/
#define TASK_STACK_SIZE     256

#define TIMER0_PRESCALER    64UL
#define TIMER0_HZ           1000UL
#define TIMER0_OCR          ((F_CPU / (TIMER0_PRESCALER * TIMER0_HZ)) - 1)

/**
 * Save the entire AVR context onto the current task's srack,
 * then store the stack pointer into the currentTask->pstack.
 * 
 * Order on stack (top to bottom after save):
 * R31..R2, R1, SREG, R0 (PC already pushed by hardware on interrupt)
 */
#define SAVE_CONTEXT()                              \
    asm volatile (                                  \
        "push r0            \n\t"                   \
        "in   r0, __SREG__  \n\t"                   \
        "cli                \n\t"                   \
        "push r0            \n\t"                   \
        "push r1            \n\t"                   \
        "clr  r1            \n\t"                   \
        "push r2            \n\t"                   \
        "push r3            \n\t"                   \
        "push r4            \n\t"                   \
        "push r5            \n\t"                   \
        "push r6            \n\t"                   \
        "push r7            \n\t"                   \
        "push r8            \n\t"                   \
        "push r9            \n\t"                   \
        "push r10           \n\t"                   \
        "push r11           \n\t"                   \
        "push r12           \n\t"                   \
        "push r13           \n\t"                   \
        "push r14           \n\t"                   \
        "push r15           \n\t"                   \
        "push r16           \n\t"                   \
        "push r17           \n\t"                   \
        "push r18           \n\t"                   \
        "push r19           \n\t"                   \
        "push r20           \n\t"                   \
        "push r21           \n\t"                   \
        "push r22           \n\t"                   \
        "push r23           \n\t"                   \
        "push r24           \n\t"                   \   
        "push r25           \n\t"                   \
        "push r26           \n\t"                   \
        "push r27           \n\t"                   \
        "push r28           \n\t"                   \
        "push r29           \n\t"                   \
        "push r30           \n\t"                   \
        "push r31           \n\t"                   \
        /* Store SP into currentTask->pstack */     \
        "lds  r26, currentTask      \n\t"           \
        "lds  r27, currentTask + 1  \n\t"           \
        "in   r0,  __SP_L__         \n\t"           \
        "st   x+,  r0               \n\t"           \
        "in   r0,  __SP_H__         \n\t"           \
        "st   x+,  r0               \n\t"           \
    );

/**
 * Restore the entire AVR context from the next task's stack.
 * Loads the SP from currentTask->pstack, then pops all registers.
 * PC is restored by the subsequent RETI.
 */
#define RESTORE_CONTEXT()                           \
    asm volatile(                                   \
        /* load SP from currentTask->pstack */      \
        "lds  r26, currentTask      \n\t"           \
        "lds  r27, currentTask + 1  \n\t"           \
        "ld   r28, x+               \n\t"           \
        "out  __SP_L__, r28         \n\t"           \
        "ld   r29, x+               \n\t"           \
        "out  __SP_H__, r29         \n\t"           \
        /* pop all registers in reverse order */    \
        "pop  r31           \n\t"                   \
        "pop  r30           \n\t"                   \
        "pop  r29           \n\t"                   \
        "pop  r28           \n\t"                   \
        "pop  r27           \n\t"                   \
        "pop  r26           \n\t"                   \
        "pop  r25           \n\t"                   \
        "pop  r24           \n\t"                   \
        "pop  r23           \n\t"                   \
        "pop  r22           \n\t"                   \
        "pop  r21           \n\t"                   \
        "pop  r20           \n\t"                   \
        "pop  r19           \n\t"                   \
        "pop  r18           \n\t"                   \
        "pop  r17           \n\t"                   \
        "pop  r16           \n\t"                   \
        "pop  r15           \n\t"                   \
        "pop  r14           \n\t"                   \
        "pop  r13           \n\t"                   \
        "pop  r12           \n\t"                   \
        "pop  r11           \n\t"                   \
        "pop  r10           \n\t"                   \
        "pop  r9            \n\t"                   \
        "pop  r8            \n\t"                   \
        "pop  r7            \n\t"                   \
        "pop  r6            \n\t"                   \
        "pop  r5            \n\t"                   \
        "pop  r4            \n\t"                   \
        "pop  r3            \n\t"                   \
        "pop  r2            \n\t"                   \
        "pop  r1            \n\t"                   \
        "pop  r0            \n\t"                   \
        "out  __SREG__, r0  \n\t"                   \
        "pop  r0            \n\t"                   \
    );

/* TYPES ***************************************************************/
typedef struct context_s {
    uint8_t           stack[TASK_STACK_SIZE]; // task's private stack
    uint8_t         * pstack;                 // saved SP
    struct context_s * next;                  // ring list pointer
} context_t;

/* PRIVATE VARIABLES ***************************************************/
static context_t * currentTask = NULL;

/* PRIVATE FUNCTIONS ***************************************************/

/**
 * TIMER0 ISR is naked so we control every push/pop ourselves.
 * Saves current context, advances round-robin, restores next context
 */
ISR(TIMER0_COMPA_vect, ISR_NAKED)
{
    SAVE_CONTEXT();

    currentTask = currentTask->next;

    RESTORE_CONTEXT();

    asm volatile("reti \n\t");
}

/**
 * Push the initial fake context for a task onto its stack so that
 * when RESTORE_CONTEXT() runs for the first time it finds a valid
 * register set and the task's entry point as "PC".
 * 
 * Stack layout built here (top = lowest address, grow down):
 *   [PC low byte]  <- pushed last, popped first by RETI
 *   [PC high byte]
 *   R0=0, SREG=0, R1=0, R2..R31=0
 */
static void context_init(context_t * ctx, task_t task)
{
    // Start SP at top of this task's stack (highest address)
    uint8_t * sp = &ctx->stack[TASK_STACK_SIZE - 1];

    // Push PC (2 bytes, low byte first on ATmega - RETI pops high first)
    uint16_t pc = (uint16_t)task;
    *sp-- = (uint8_t)(pc & 0xFF);           // PC low
    *sp-- = (uint8_t)((pc >> 8) & 0xFF);    // PC high

    /* Push fake register context: R0, SREG, R1, R2..R31 = all zeros
       Order must match SAVE_CONTEXT push order */
    *sp-- = 0;  // R0
    *sp-- = 0;  // SREG
    *sp-- = 0;  // R1
    for (uint8_t i = 0; i < 31; i++) {
        *sp-- = 0;  // R2..R31
    }

    // Save SP - pstack points to current top of this fake context
    ctx->pstack = sp + 1;
}

/* PUBLIC FUNCTIONS ************************************************/
void pscheduler_run(const task_t * taskArray, uint8_t len)
{
    /* Allocate task context statically = array sized at compile time
       via the maximum tasks we support */
    static context_t contexts[8];   // Supports up to 8 tasks

    // Build ring list and initialize each context
    for (uint8_t i = 0; i < len; i++) {
        context_init(&contexts[i], taskArray[i]);
        contexts[i].next = &contexts[(i + 1) % len];
    }

    // Set up TIMER0 in CTC mode, 1ms tick
    TCCR0A = (1 << WGM01);
    OCR0A = (uint8_t)TIMER0_OCR;
    TIFR0 |= (1 << OCF0A);
    TIMSK0 |= (1 << OCIE0A);
    TCCR0B = (1 << CS01) | (1 << CS00);

    // Pick first task and restore its context to start execution
    currentTask = &contexts[0];

    // Restore first task's context and jump into it via RETI
    RESTORE_CONTEXT();
    asm volatile("reti \n\t");
}