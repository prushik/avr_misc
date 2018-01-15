// This should definitely be defined in the application or like a config header or some shit.
#ifndef F_CPU
	#define __AVR_ATmega328P__ 1
	#define F_CPU 16000000UL
#endif

#ifndef BAUD
	#define BAUD 9600
#endif

#include <avr/io.h>
#include <util/setbaud.h>
#include "uart.h"

//init
void uart_init()
{
	UBRR0H = UBRRH_VALUE;
	UBRR0L = UBRRL_VALUE;

#if USE_2X
	UCSR0A |= _BV(U2X0);
#else
	UCSR0A &= ~(_BV(U2X0));
#endif

	UCSR0C = _BV(UCSZ01) | _BV(UCSZ00); /* 8-bit data */
	UCSR0B = _BV(RXEN0) | _BV(TXEN0);   /* Enable RX and TX */
}

//blocking
void uart_putchar(unsigned char c)
{
	loop_until_bit_is_set(UCSR0A, UDRE0); /* Wait until data register empty. */
	UDR0 = c;
	loop_until_bit_is_set(UCSR0A, TXC0); /* Wait until transmission ready. */
}

// same thing, but don't block
void uart_send_char(unsigned char c)
{
	UDR0 = c;
}

//blocking
void uart_write(char *data, unsigned int len)
{
	int i;
	for (i=0; i < len; i++)
	{
		uart_putchar(data[i]);
	}
}

//blocking
char uart_getchar()
{
	loop_until_bit_is_set(UCSR0A, RXC0); /* Wait until data exists. */
	return UDR0;
}

//nonblocking
char uart_read_char(unsigned char *a)
{
	if (UCSR0A & _BV(RXC0))
	{
		*a = UDR0;
		return 1;
	}
	else
	{
		return 0;
	}
}
