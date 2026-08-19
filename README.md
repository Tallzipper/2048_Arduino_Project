# Embedded 2048 Game on ATmega328P

A bare-metal C implementation of the 2048 puzzle game built for an ATmega328P microcontroller. This project runs on hardware without external framework libraries.

---

| System | Technology / Implementation |
| :--- | :--- |
| **Language & Platform** | Bare-Metal C / ATmega328P |
| **Primary Display** | SPI ST7735R TFT LCD (128x160) |
| **Scoreboard** | 4-Digit 7-Segment Display (2x 74HC595 Shift Registers) |
| **Input Handling** | 2-Axis Analog Joystick (ADC-driven) |
| **Task Execution** | 7 Concurrent SynchSMs (10ms Global Period) |

## Demonstration

[Demo](https://www.youtube.com/watch?v=NrkG_aW3MAA)

Gif will be added showing gameplay, state updates, and menu controls.

## Hardware

- ATmega328P (Arduino Uno R3 Board)
- HiLetGo 1.8" ST7735R SPI 128x160 TFT LCD
- 4-Digit 7-Segment Display
- 2x SN74HC595N IC 8-Bit Shift Registers
- 2-Axis Analog Joystick
- 2x Push Buttons
- 10K Potentiometer
- Passive Buzzer
- Green LED
- 3x 330 OHM Resistors
- 830 Tie-Points Breadboard & Wires

## Wiring Diagram

- Will be added momentarily, image exists, just needs to be added

## Task Diagram / Firmware Architecture

- Will be added momentarily, image exists, just needs to be added

## User Guide

- **Starting the Game:** Press the left button for any duration to set up and turn on the game.
- **Moving Blocks:** Use the joystick to slide up, down, left, or right. The screen only updates if the move is valid (blocks shift or merge).
- **Undoing a Move:** Press the left button for less than 3 seconds during a run to undo your last move. This option resets once a new move is made.
- **Restarting:** Press the right button to reset your current run back to the starting board.
- **Power Off:** Hold the left button for 3 seconds or longer to clear the board and turn the screen black.
- **Score & End Game:** The 4D7S display shows your highest block value. Reaching 2048 lights up the top LED. Running out of moves displays a red screen for loss or green screen for victory before resetting.

## Known Issues

- Camera recordings may show flicker or brightness differences on the 4-digit 7-segment display due to frame-rate capturing, though it appears perfectly normal in person.
- Short `_delay_us` loops ($2\text{ ms}$) are used within segment multiplexing routines to prevent ghosting on the 4D7S screen while maintaining the $10\text{ ms}$ global task period.
- Two tasks write to the same `boardChange` signal variable, but both write a constant value (`1`) to trigger screen redraws without memory conflict.

## Authors Note

This project was made over the course of four weeks through the use of a lot of technologies I am unfamiliar with, so the code is very rough. One day I'll look over the code and optimize it but for now it is well documented and works perfectly so it will be left as is
