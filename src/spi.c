#include <avr/io.h>

void spi_init()
{
	SPCR |= ( (1<<SPE) | (1<<MSTR) ); // enable SPI as master
	//SPCR |= ( (1<<SPR1) | (1<<SPR0) ); // set prescaler bits
	SPCR &= ~( (1<<SPR1) | (1<<SPR0) ); // clear prescaler bits
	SPSR = 0; // clear SPI status reg
	SPDR = 0; // clear SPI data reg
//	SPSR |= (1<<SPI2X); // set prescaler bits
	//SPSR &= ~(1<<SPI2X); // clear prescaler bits
}

/*
void spi_transfer(unsigned char data)
{
	int i;
	for (i=0; i<8; i++)
	{
		if (data & 1)
			PORTB |= SHIFT_DATA;
		else
			PORTB &= ~SHIFT_DATA;

		data = data>>1;

		PORTB |= SHIFT_CLOCK;
		PORTB &= ~SHIFT_CLOCK;
	}
}*/

char spi_transfer(char data)
{
	SPDR = data;					// Start the transmission
	while (!(SPSR & (1<<SPIF))) ;	// Wait the end of the transmission
	return SPDR;					// return the received byte, we don't need that
}
