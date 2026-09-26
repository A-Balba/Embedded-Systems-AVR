# Embedded Systems on AVR

## C & Real-Time Systems

Portfolio reconstruction of selected embedded-systems work developed on an AVR-based platform during the **Software for Embedded Systems** course at **TU Hamburg (TUHH)**.

The repository focuses on the implementations and engineering concepts that are most relevant to embedded and mechatronics work rather than reproducing the original course workspace.

## Highlights

- Low-level AVR peripheral programming in C
- GPIO, LEDs, push buttons and rotary-encoder input
- Timer and interrupt-based timing
- ADC acquisition and sensor input processing
- Cooperative task scheduling
- Preemptive multitasking and context switching
- PWM-based fan actuation
- Tachometer-based RPM measurement and filtering
- Event-driven finite-state-machine design
- Integration of peripherals, scheduling and application logic

## Architecture

```text
                    +----------------------+
                    |   Application Logic   |
                    | Fan Controller / FSM  |
                    +----------+-----------+
                               |
                    +----------v-----------+
                    |   Task Scheduling    |
                    | Cooperative /        |
                    | Preemptive           |
                    +----------+-----------+
                               |
          +--------------------+--------------------+
          |                    |                    |
+---------v---------+  +-------v--------+  +--------v---------+
|   Input / ADC     |  | Timer / GPIO   |  | Fan / Tachometer |
| Buttons, ADC      |  | Interrupts     |  | PWM, RPM, filter |
+-------------------+  +----------------+  +------------------+
                               |
                    +----------v-----------+
                    |      AVR Board       |
                    +----------------------+
```

A more detailed view is available in [`docs/architecture.md`](docs/architecture.md).

## Selected Implementations

### 1. AVR Peripheral Drivers

The `peripherals/` directory contains reusable C modules for ADC acquisition, button input, LED control and timer services.

The button implementation uses timer-driven state checking for debouncing and supports callbacks for user input events.

### 2. Cooperative Scheduler

The scheduler provides periodic and one-shot task execution using timer-based timing. Tasks are represented by descriptors and managed without requiring a full preemptive operating system.

### 3. Fan Actuation & Speed Measurement

The fan-control modules combine PWM actuation with tachometer feedback. The measured rotational speed is processed with a median filter before being used by the application for monitoring.

The corresponding application reads a potentiometer through the ADC, maps the reading to an 8-bit duty cycle, switches the fan through a button callback, and periodically reports measured and filtered speed together with the duty cycle. The tachometer path provides measurement/feedback; the application does not implement automatic closed-loop speed regulation.

### 4. Preemptive Scheduler

The `preemptive_scheduler/` directory contains the preemptive scheduling implementation developed as part of the embedded-systems work, including task context switching and AVR-specific low-level operations.

### 5. Alarm Clock FSM

The alarm-clock application demonstrates event-driven application architecture. Its logic is implemented as a finite-state machine with states for time initialization, normal operation, alarm configuration and alarm ringing.

It integrates scheduler tasks, button/rotary events, LEDs and time handling.

> The display interface used by the original application was provided as course infrastructure. The external display implementation is intentionally not included in this portfolio repository.

## Repository Structure

```text
Embedded-Systems-AVR/
├── README.md
├── docs/
│   └── architecture.md
└── src/
    ├── peripherals/
    │   ├── ses_adc.c/.h
    │   ├── ses_button.c/.h
    │   ├── ses_led.c/.h
    │   └── ses_timer.c/.h
    ├── scheduling/
    │   └── ses_scheduler.c/.h
    ├── fan_control/
    │   ├── ses_fan.c/.h
    │   └── ses_fanspeed.c/.h
    ├── preemptive_scheduler/
    │   ├── pscheduler.c
    │   └── pscheduler.h
    └── applications/
        ├── fan_controller.c
        ├── alarm_clock_main.c
        ├── alarm_clock.c
        └── alarm_clock.h
```

## Hardware & Software

- AVR-based embedded-systems training platform
- C
- AVR-GCC / C development environment
- Original course workspace used PlatformIO for project/build management
- Hardware peripherals: GPIO, timers, ADC, PWM, buttons and tachometer input

## Course Context

This repository is a **portfolio reconstruction** of selected work from TUHH's Software for Embedded Systems course. It is intentionally not a copy of the original submission workspace.

Course sheets, university templates, generated build directories, compiled artifacts, precompiled course libraries, and external driver implementations are excluded. The repository therefore emphasizes the embedded concepts and source implementations that can be presented independently.

## Contribution

The implementations in this repository represent my work on the selected embedded-systems components and applications developed during the course.

For application modules that depended on course-provided infrastructure, only the application-side logic and integration code are included here.

## Note on Reproducibility

Some application files depend on interfaces supplied by the original course environment, particularly the display interface. Consequently, this repository should be read as a curated portfolio of the embedded implementations rather than as a guaranteed drop-in replacement for the original TUHH course workspace.
