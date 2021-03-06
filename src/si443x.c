// si443x (and boards using one, like xl4432 and rf-uh4432) communicate through SPI
#include "config.h"

#ifndef X86
	#include <avr/io.h>
#else
	#include <stdio.h>
	#include <math.h>
	#include <stdlib.h>
	#include <stdint.h>
	#include <sys/types.h>
	char PORTB;

	char spi_transfer(char data) {printf("spi: 0x%02x\n", data);}
	char output_device[128] = {0};
#endif

#include "si443x.h"
#include "spi.h"

void si443x_write(uint8_t reg, const uint8_t *value, uint8_t len)
{
	uint8_t i;

	SI443X_SS_PORT |= SI443X_SS_BIT;
	spi_transfer(0x80 | reg);
	for (i = 0; i < len; i++)
	{
		#ifdef X86
			output_device[reg+i] = value[i];
		#endif
		spi_transfer(value[i]);
	}
	SI443X_SS_PORT &= ~SI443X_SS_BIT;
}

void si443x_read(uint8_t reg, uint8_t *out, uint8_t len)
{
	uint8_t i;

	SI443X_SS_PORT |= SI443X_SS_BIT;
	spi_transfer((~0x80) & reg);
	out[0] = spi_transfer(len);
	for (i = 1; i < len; i++)
	{
		out[i] = spi_transfer(0x00);
	}
	SI443X_SS_PORT &= ~SI443X_SS_BIT;
}

#ifdef X86
	void readall()
	{
		int i;
		printf("REGISTERS: ");
		for (i=0; i< 0x7F; i++)
		{
			printf("0x%02x ", i);
		}
		printf("\n");
		printf("READALL: ");
		for (i=0; i< 0x7F; i++)
		{
			printf("0x%02x ", output_device[i]);
		}
		printf("\n");
	}
#endif

// SPI should be initialized first
// SPI interface has 10mhz max
void si443x_init()
{
	uint8_t vals[6];

	vals[0] = 0x3c; vals[1] = 0x02;
	si443x_write(SI443X_REG_AFC_LOOP_GEARSHIFT_OVERRIDE, vals, 2);
	// REG_AFC_TIMING_CONTROL

	vals[0] = 0xff;
	si443x_write(SI443X_REG_AFC_LIMITER, vals, 1);

	vals[0] = 0xad;
	si443x_write(SI443X_REG_DATAACCESS_CONTROL, vals, 1);

	vals[0] = 0x0c;	vals[1] = 0x22;	vals[2] = 0x08;	vals[3] = 0x3a;	vals[4] = 0x2d;	vals[5] = 0xd4;
	si443x_write(SI443X_REG_HEADER_CONTROL1, vals, 6);
	// SI443X_REG_HEADER_CONTROL2
	// SI443X_REG_PREAMBLE_LENGTH
	// SI443X_REG_PREAMBLE_DETECTION
	// SI443X_REG_SYNC_WORD3
	// SI443X_REG_SYNC_WORD2

	vals[0] = 0x60;
	si443x_write(SI443X_REG_AGC_OVERRIDE, vals, 1);

	vals[0] = 0x1f;
	si443x_write(SI443X_REG_TX_POWER, vals, 1);

	vals[0] = 0x64;
	si443x_write(SI443X_REG_CHANNEL_STEPSIZE, vals, 1);

	si443x_set_frequency_mhz(433);

	si443x_set_hw_address("PHIL", 4);

	si443x_set_baud(100);

	vals[0] = 0x0; // channel 0
	si443x_write(SI443X_REG_FREQCHANNEL, vals, 1);

	si443x_set_mode(SI443X_MODE_READY);
}

void si443x_set_mode(uint8_t mode)
{
//	uint8_t _txPin = 21; // not sure what these were for... maybe leds?
//	uint8_t _rxPin = 23;

	if (mode & SI443X_MODE_TX) {
//		digitalWrite(_txPin, 1);
//		digitalWrite(_rxPin, 0);
	}
	if (mode & SI443X_MODE_RX) {
//		digitalWrite(_txPin, 0);
//		digitalWrite(_rxPin, 1);
	}

	si443x_write(SI443X_REG_STATE, &mode, 1);

//	delay(20);
}

void si443x_set_frequency_hz(unsigned long int freq)
{
	if ((freq < 240E6) || (freq > 930E6))
		return; // invalid frequency

	char hbsel = (freq >= 480E6);

	uint16_t freqNF = ((uint16_t) freq / (10000000 << hbsel)) - 24;
	uint8_t freqFB = (uint8_t) freqNF; // truncate the int
	uint16_t freqFC = (uint16_t) ((int)(freqNF - freqFB) * (6400>>hbsel));

	uint8_t vals[3] = { 0x40 | (hbsel << 5) | (freqFB & 0x3F), freqFC >> 8, freqFC & 0xFF };
	si443x_write(SI443X_REG_FREQBAND, vals, 3);
}

void si443x_set_frequency_mhz(uint16_t freq_mhz)
{
	if ((freq_mhz < 240) || (freq_mhz > 930))
		return; // invalid frequency

	unsigned char hbsel = (freq_mhz >= 480);

	uint16_t freqNF = (freq_mhz / (10 << hbsel)) - 24;
	uint16_t freqFC = (freq_mhz % (10 << hbsel))*(6400>>hbsel);

	unsigned char vals[3] = { 0x40 | (hbsel << 5) | (freqNF & 0x3F), freqFC >> 8, freqFC & 0xFF };
	si443x_write(SI443X_REG_FREQBAND, vals, 3);
}

void si443x_set_hw_address(uint8_t *addr, uint8_t len)
{
	si443x_write(SI443X_REG_TRANSMIT_HEADER3, addr, len);
	si443x_write(SI443X_REG_CHECK_HEADER3, addr, len);
}


void si443x_set_baud(uint16_t kbps)
{
	// chip normally supports very low bps values, but they are cumbersome to implement - so I just didn't implement lower bps values
	if ((kbps > 256) || (kbps < 1))
		return;

	uint8_t freq_dev = (kbps <= 10 ? 15 : 150);
	uint8_t vals[3] = {
		(kbps < 30 ? 0x4c : 0x0c), 
		0x23, 
		(kbps <= 10 ? 24 : 240)
	};
	si443x_write(SI443X_REG_MODULATION_MODE1, vals, 3);

	// set data rate
	uint16_t bps_reg = (kbps << (kbps < 30 ? 21 : 16))/1000; // this can definitely overflow
	vals[0] = bps_reg >> 8;
	vals[1] = bps_reg & 0xff;

	si443x_write(SI443X_REG_TX_DATARATE1, vals, 2);

	//now set the timings
	uint16_t min_bandwidth = ((freq_dev<<1) + kbps) << 6;

	printf("min_bw %lf \n", (double)min_bandwidth/64.0);

	uint8_t IFValue = 0xff;

	uint8_t ndec_exp = 5;
	uint8_t dwn3_bypass = 0;
	uint8_t filset = 1;
	uint16_t bw = 167; // 2.6<<7
	uint8_t bw_ = 0; // 2.6<<7

	uint8_t row=0;
	for (row=0; row < 64; row++)
	{
		printf("freq %lf \n", (double)bw/64.0 );

		if (bw > min_bandwidth)
		{
			if (row < 42)
			{
				ndec_exp = 6 - (row / 7);
				dwn3_bypass = 0;
				filset = 1 + (row % 7);
			}
			else
			{
				ndec_exp = row <= 44;
				dwn3_bypass = 1;
				filset = 1 + (((row+6)+(row>44?8:0)+(row>49?3:0)) % 15);
			}
			goto found_filter;
		}

		bw_ = (bw >> 7); // we need this because the intermediate value won't fit in 16 bits, so we need to split up the job into 2 parts
		bw = (bw_ * 141) + (((bw & 0x7f) * 141) >> 7);
	}

found_filter:
#ifdef X86
  printf("dwn3_bypass value: %#02x\n", dwn3_bypass);
  printf("ndec_exp value: %#02x\n", ndec_exp);
  printf("filset value: %#02x\n", filset);
#endif
	IFValue = dwn3_bypass << 7 | (ndec_exp & 0x03) << 5 | filset;
	si443x_write(SI443X_REG_IF_FILTER_BW, &IFValue, 1);

#ifdef DEBUG
	printf("Selected IF value: %#04x\n", IFValue);
#endif

// these should already be set
//	byte dwn3_bypass = (IFValue & 0x80) ? 1 : 0; // if msb is set
//	byte ndec_exp = (IFValue >> 4) & 0x07; // only 3 bits

	// todo, these need to be converted into integer maths
	uint16_t rxOversampling = round((500.0 * (1 + 2 * dwn3_bypass)) / (pow(2, ndec_exp-3) * (double) kbps));

	uint32_t ncOffset = ceil(((double) kbps * (pow(2, ndec_exp + 20))) / (500.0 * (1 + 2 * dwn3_bypass)));

	uint16_t crGain = 2 + ((65535 * (int64_t) kbps) / ((int64_t) rxOversampling * freq_dev));
	uint8_t crMultiplier = 0x00;
	if (crGain > 0x7FF)
		crGain = 0x7FF;

#ifdef X86
	printf("dwn3_bypass value: %#04x\n", dwn3_bypass);
	printf("ndec_exp value: %#04x\n", ndec_exp);
	printf("rxOversampling value: %#04x\n", rxOversampling);
	printf("ncOffset value: %#04x\n", ncOffset);
	printf("crGain value: %#04x\n", crGain);
	printf("crMultiplier value: %#04x\n", crMultiplier);
#endif

	uint8_t timingVals[] = { rxOversampling & 0x00FF, ((rxOversampling & 0x0700) >> 3) | ((ncOffset >> 16) & 0x0F),
	  (ncOffset >> 8) & 0xFF, ncOffset & 0xFF, ((crGain & 0x0700) >> 8) | crMultiplier, crGain & 0xFF };

	si443x_write(SI443X_REG_CLOCK_RECOVERY_OVERSAMPLING, timingVals, 6);
}


bool sendPacket(uint8_t length, const byte* data, bool waitResponse, uint32_t ackTimeout,
	uint8_t* responseLength, byte* responseBuffer)
{

	si443x_clear_fifo(SI443X_FIFO_TX);
	si443x_write(SI443X_REG_PKG_LEN, length, 1);

	si443x_write(SI443X_REG_FIFO, data, length);

	ChangeRegister(REG_INT_ENABLE1, 0x04); // set interrupts on for package sent
	ChangeRegister(REG_INT_ENABLE2, 0x00); // set interrupts off for anything else
	//read interrupt registers to clean them
	ReadRegister(REG_INT_STATUS1);
	ReadRegister(REG_INT_STATUS2);

	switchMode(TXMode | Ready);

	uint64_t enterMillis = millis();

	while (millis() - enterMillis < 200) {

		byte intStatus = ReadRegister(REG_INT_STATUS1);
		ReadRegister(REG_INT_STATUS2);

		if (intStatus & 0x04) {
			si443x_set_mode(SI443X_MODE_READY | SI443X_MODE_TUNE);
			#ifdef DEBUG
			  printf("Package sent! -- ");
			  printf("%#04x\n", intStatus);
			#endif

			// package sent. now, return true if not to wait ack, or wait ack (wait for packet only for 'remaining' amount of time)
			if (waitResponse) {
				if (waitForPacket(ackTimeout)) {
					getPacketReceived(responseLength, responseBuffer);
					return true;
				} else {
					return false;
				}
			} else {
				return true;
			}
		}
	}

	//timeout occured.
	//#ifdef DEBUG
	printf("Timeout in Transit -- \n");
	//#endif
	si443x_set_mode(SI443X_MODE_READY);

	if (ReadRegister(REG_DEV_STATUS) & 0x80) {
		si443x_clear_fifo(SI443X_FIFO_RX | SI443X_FIFO_TX);
	}

	return false;
}

void si443x_clear_fifo(uint8_t which)
{
	si443x_write(SI443X_REG_OPERATION_CONTROL, &which, 1);
	si443x_write(SI443X_REG_OPERATION_CONTROL, 0x00,   1);
}

#ifdef X86
int main()
{
	si443x_init();
	readall();
}
#endif
