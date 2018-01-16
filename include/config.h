// --- general MCU speed settings ---
#ifndef F_CPU
	#define __AVR_ATmega328P__ 1
	#define F_CPU 16000000UL
#endif

// --- uart settings ---
#ifndef BAUD
	#define BAUD 9600
#endif

// --- si443x settings ---
// default is pin 10
#ifndef SI443X_SS_PORT
	#define SI443X_SS_PORT PORTB
#endif
#ifndef SI443X_SS_DDR
	#define SI443X_SS_DDR DDRB
#endif
#ifndef SI443X_SS_BIT
	#define SI443X_SS_BIT 0x04
#endif
