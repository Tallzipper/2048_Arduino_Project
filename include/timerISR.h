// Permission to copy is granted provided that this header remains intact. 
// This software is provided with no warranties.

#ifndef TIMER_H
#define TIMER_H

#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>


volatile unsigned char TimerFlag = 0; // TimerISR() sets this to 1. C programmer should clear to 0.

// Internal variables for mapping AVR's ISR to our cleaner TimerISR model.
unsigned long _avr_timer_M = 1; // Start count from here, down to 0. Default 1ms
unsigned long _avr_timer_cntcurr = 0; // Current internal count of 1ms ticks

void TimerISR(void);

// Set TimerISR() to tick every M ms
void TimerSet(unsigned long M) {
	_avr_timer_M = M;
	_avr_timer_cntcurr = _avr_timer_M;
}

void TimerOn() {
	// AVR timer/counter controller register TCCR1
	TCCR2A = 0x02;  // Used to be 0x00
  	TCCR2B 	= 0x04;	// Used to be 0x0B
					// bit3 = 1: CTC mode (clear timer on compare)
					// bit2bit1bit0=011: prescaler /64
					// 00001011: 0x0B
					// SO, 16 MHz clock or 16,000,000 /64 = 250,000 ticks/s
					// Thus, TCNT1 register will count at 250,000 ticks/s
          // FOR MICROSECONDS:
          // bit3 = 1: CTC mode (clear timer on compare)
					// bit2bit1bit0=010: prescaler /64
					// 00001010: 0x0A
					// SO, 16 MHz clock or 16,000,000 /8 = 2,000,000 ticks/s
					// Thus, TCNT1 register will count at 2,000,000 ticks/s

	// AVR output compare register OCR1A.
	OCR2A 	= 250;	// Timer interrupt will be generated when TCNT1==OCR1A
					// We want a 1 ms tick. 0.001 s * 250,000 ticks/s = 250
					// So when TCNT1 register equals 250,
					// 1 ms has passed. Thus, we compare to 250.
					// AVR timer interrupt mask register
          // FOR MICROSECONDS:
          // Timer interrupt will be generated when TCNT1==OCR1A
					// We want a 1 us tick. 0.000001 s * 2,000,000 ticks/s = 2
					// So when TCNT1 register equals 2,
					// 1 us has passed. Thus, we compare to 2.
					// AVR timer interrupt mask register

	TIMSK2 	= 0x02; // bit1: OCIE1A -- enables compare match interrupt

	//Initialize avr counter
	TCNT2 = 0;

	// TimerISR will be called every _avr_timer_cntcurr milliseconds
	_avr_timer_cntcurr = _avr_timer_M;

	//Enable global interrupts
	SREG |= 0x80;	// 0x80: 1000000
}

void TimerOff() {
	TCCR2B 	= 0x00; // bit3bit2bit1bit0=0000: timer off
}



// In our approach, the C programmer does not touch this ISR, but rather TimerISR()
ISR(TIMER2_COMPA_vect)
{
	// CPU automatically calls when TCNT0 == OCR0 (every 1 ms per TimerOn settings)
	_avr_timer_cntcurr--; 			// Count down to 0 rather than up to TOP
	if (_avr_timer_cntcurr == 0) { 	// results in a more efficient compare
		TimerISR(); 				// Call the ISR that the user uses
		_avr_timer_cntcurr = _avr_timer_M;
	}

}

int TimerOverflow = 0;

ISR(TIMER1_OVF_vect)
{
	TimerOverflow++;	/* Increment Timer Overflow count */
}

// --- Timer 1 Music Setup ---
volatile unsigned int toggle_count = 0;

// This interrupt fires at twice the frequency of the note to toggle the buzzer pin
ISR(TIMER1_COMPA_vect) {
    if (toggle_count > 0) {
        PORTB ^= (1 << PORTB4); // Toggle B4 (turns sound on and off to create the wave)
        toggle_count--;
    } else {
        PORTB &= ~(1 << PORTB4); // Ensure B4 is low when the note ends
    }
}

// Function to set Timer 1 to a specific musical frequency
void play_frequency(unsigned int frequency, unsigned int duration_ms) {
    if (frequency == 0) {
        // Silence: turn off Timer 1 interrupts
        TIMSK1 &= ~(1 << OCIE1A);
        PORTB &= ~(1 << PORTB4);
        return;
    }

    // Calculate the number of pin toggles required for the given duration
    // Formula: Toggles = 2 * frequency * (duration_ms / 1000)
    toggle_count = ((unsigned long)2 * frequency * duration_ms) / 1000;

    // Set Timer 1 to CTC Mode (WGM12 = 1) and set Prescaler to 64 (CS11 and CS10 = 1)
    TCCR1A = 0x00;
    TCCR1B = (1 << WGM12) | (1 << CS11) | (1 << CS10);

    // Calculate OCR1A value for the frequency
    // Formula: OCR1A = (F_CPU / (2 * Prescaler * Frequency)) - 1
    // For 16MHz clock and 64 prescaler: 16000000 / (2 * 64 * freq) = 125000 / freq
    OCR1A = (125000 / frequency) - 1;

    TCNT1 = 0;              // Reset counter
    TIMSK1 |= (1 << OCIE1A); // Enable Compare Match A Interrupt
}


#endif // TIMER_H
