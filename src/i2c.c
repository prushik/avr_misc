#include "config.h"

#include <avr/io.h>
#include "i2c.h"

void i2c_init()
{
	//set SCL to 400kHz
	TWSR = 0x00;
	TWBR = 0x11;

	//enable TWI
	TWCR = (1<<TWEN);
}

void i2c_start()
{
	TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);
	while ((TWCR & (1<<TWINT)) == 0);
}

//send stop signal
void i2c_stop()
{
	while (TWCR & (1<<TWIE)) ;
	TWCR = (1<<TWINT)|(1<<TWSTO)|(1<<TWEN);
//	while (TWCR & (1<<TWSTO)) ;
}

void i2c_write(uint8_t data)
{
	TWDR = data;
	TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWEA);
	while ((TWCR & (1<<TWINT)) == 0);
	_delay_ms(12);
}

uint8_t i2c_read_ack()
{
	TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWEA);
	while ((TWCR & (1<<TWINT)) == 0);
	return TWDR;
}

//read byte with NACK
uint8_t i2c_read_nack()
{
	TWCR = (1<<TWINT)|(1<<TWEN);
	while ((TWCR & (1<<TWINT)) == 0);
	return TWDR;
}

uint8_t i2c_get_status()
{
	return TWSR & 0xF8;
}

uint8_t i2c_get_status_()
{
	return TWSR & 0xf8;
}

void i2c_scan()
{
	char i;
	for (i = 0; i < 0x7f; i++)
	{
		i2c_start();
		i2c_write((i << 1) | 0);
//		_delay_ms(12);
		char ack = i2c_get_status();
		i2c_stop();

		// 0x18 means ack
		// 0x20 means nack
		if (ack == 0x18)
		{
			uart_write("Found i2c device at: ", 21);
			uart_putchar(inttohex[i>>4]);
			uart_putchar(inttohex[i&0x0f]);
			uart_putchar('\n');
		}
	}
}
