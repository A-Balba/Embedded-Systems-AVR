# Embedded Systems Architecture

## Overview

The project is organized around four layers:

1. **Peripheral layer** — hardware-facing modules for ADC, buttons, LEDs and timers.
2. **Scheduling layer** — periodic and one-shot task execution, plus a separate preemptive scheduler implementation.
3. **Control / feedback layer** — PWM fan actuation, tachometer measurement and RPM filtering.
4. **Application layer** — application-specific task logic and the alarm-clock finite-state machine.

```text
                         APPLICATIONS
                +---------------------------+
                | Fan Controller             |
                | Alarm Clock FSM             |
                +-------------+-------------+
                              |
                              v
                       TASK SCHEDULING
                +---------------------------+
                | Cooperative Scheduler      |
                | Preemptive Scheduler       |
                +-------------+-------------+
                              |
             +----------------+----------------+
             |                |                |
             v                v                v
       INPUT / ADC       TIMING / GPIO     FAN FEEDBACK
       +-----------+     +------------+    +-------------+
       | ADC       |     | Timer      |    | PWM         |
       | Buttons   |     | LEDs       |    | Tachometer  |
       +-----------+     +------------+    | RPM filter  |
                                           +-------------+
                              |
                              v
                       AVR HARDWARE
```

## Peripheral Layer

### ADC

`ses_adc.c/.h` provides ADC initialization, channel selection and conversion services used by applications such as the potentiometer-based fan controller.

### Buttons

`ses_button.c/.h` handles button and rotary-encoder input, including callback registration and timer-assisted state checking/debouncing.

### LEDs

`ses_led.c/.h` provides the LED control used by application feedback and state indication.

### Timers

`ses_timer.c/.h` provides timer initialization and timer callback services used by the scheduler and input handling.

## Scheduling Layer

### Cooperative Scheduler

`ses_scheduler.c/.h` manages task descriptors and executes periodic or one-shot tasks according to their expiration and period values. Timing is driven by the timer subsystem.

### Preemptive Scheduler

`pscheduler.c/.h` contains the preemptive multitasking implementation, including task context management and AVR-specific low-level context switching.

These two schedulers are kept as separate modules because they demonstrate different approaches to real-time task execution.

## Fan Feedback Path

```text
Potentiometer
     |
     v
    ADC -----> Duty-cycle command -----> PWM -----> Fan
                                                |
                                                v
                                          Tachometer
                                                |
                                                v
                                      RPM measurement
                                                |
                                                v
                                         Median filter
                                                |
                                                v
                                         RPM feedback
```

The fan application periodically samples the potentiometer, converts the ADC value into an 8-bit duty-cycle command and applies it when the fan is enabled. Tachometer measurements provide rotational-speed feedback, with a median-filtered value used for a more stable displayed measurement.

## Alarm Clock Application

The alarm clock uses an event-driven finite-state machine. Its states include:

- initial hour configuration
- initial minute configuration
- normal clock operation
- alarm-hour configuration
- alarm-minute configuration
- alarm ringing

Events are generated from button/rotary input and periodic scheduler tasks. State transitions update the display and LED indicators and control alarm timing.

The original application also used a course-provided display interface. That external implementation is intentionally excluded from this repository.
