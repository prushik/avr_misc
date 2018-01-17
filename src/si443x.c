// si443x (and boards using one, like xl4432 and rf-uh4432) communicate through SPI
#include "config.h"
#include "si443x.h"
#include "spi.h"

void si443x_write(uint8_t reg, const uint8_t *value, uint8_t len)
{
	uint8_t i;

	SI443X_SS_PORT |= SI443X_SS_BIT;
	spi_transfer(0x80 | reg);
	for (i = 0; i < len; i++)
	{
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


// SPI should be initialized first
// SPI interface has 10mhz max
void si443x_init()
{
	si443x_write(SI443X_REG_AFC_LOOP_GEARSHIFT_OVERRIDE, {0x3c, 0x02}, 2);
	// REG_AFC_TIMING_CONTROL
	si443x_write(SI443X_REG_AFC_LIMITER, {0xff}, 1);
	si443x_write(SI443X_REG_DATAACCESS_CONTROL, {0xad}, 1);
	si443x_write(SI443X_REG_HEADER_CONTROL1, {0x0c, 0x22, 0x08, 0x3a, 0x2d, 0xd4}, 6);
	// SI443X_REG_HEADER_CONTROL2
	// SI443X_REG_PREAMBLE_LENGTH
	// SI443X_REG_PREAMBLE_DETECTION
	// SI443X_REG_SYNC_WORD3
	// SI443X_REG_SYNC_WORD2
	si443x_write(SI443X_REG_AGC_OVERRIDE, {0x60, 0x1f}, 2);
	// SI443X_REG_TX_POWER
	si443x_write(SI443X_REG_CHANNEL_STEPSIZE, {0x64}, 1);

	
/*
	ChangeRegister(REG_AFC_LOOP_GEARSHIFT_OVERRIDE, 0x3C); // turn off AFC
	ChangeRegister(REG_AFC_TIMING_CONTROL, 0x02);

	ChangeRegister(REG_AFC_LIMITER, 0xFF);

	ChangeRegister(REG_DATAACCESS_CONTROL, 0xAD); // enable rx packet handling, enable tx packet handling, enable CRC, use CRC-IBM

	ChangeRegister(REG_HEADER_CONTROL1, 0x0C); // no broadcast address control, enable check headers for bytes 3 & 2
	ChangeRegister(REG_HEADER_CONTROL2, 0x22);  // enable headers byte 3 & 2, no fixed package length, sync word 3 & 2
	ChangeRegister(REG_PREAMBLE_LENGTH, 0x08); // 8 * 4 bits = 32 bits (4 bytes) preamble length
	ChangeRegister(REG_PREAMBLE_DETECTION, 0x3A); // validate 7 * 4 bits of preamble  in a package
	ChangeRegister(REG_SYNC_WORD3, 0x2D); // sync byte 3 val
	ChangeRegister(REG_SYNC_WORD2, 0xD4); // sync byte 2 val

	ChangeRegister(REG_AGC_OVERRIDE, 0x60);
	ChangeRegister(REG_TX_POWER, 0x1F); // max power

	ChangeRegister(REG_CHANNEL_STEPSIZE, 0x64); // each channel is of 1 Mhz interval

	setFrequency(_freqCarrier); // default freq
	setBaudRate(_kbps); // default baud rate is 100kpbs
	setChannel(_freqChannel); // default channel is 0
	setCommsSignature(_packageSign); // default signature
*/
	si443x_set_mode(Ready);
}

void si443x_set_mode(byte mode) {
	uint8_t _txPin = 21; // not sure what these were for... maybe leds?
	uint8_t _rxPin = 23;

	if (mode & SI443X_MODE_TX) {
		digitalWrite(_txPin, 1);
		digitalWrite(_rxPin, 0);
	}
	if (mode & SI443X_MODE_RX) {
		digitalWrite(_txPin, 0);
		digitalWrite(_rxPin, 1);
	}

	si443x_write(SI443X_REG_STATE, &mode, 1);

	delay(20);
}

void si443x_set_frequency(unsigned long int freq) {
	if ((freq < 240E6) || (freq > 930E6)) {
	//    printf("error: invalid frequency: %lu\n", freqTX);
	return; // invalid frequency
	}

	// set internal variable
	//  _freqCarrier = freqTX;

	char hbsel = (freq >= 480E6);

	uint16_t freqNF = ((uint16_t) freq / (10E6 << hbsel)) - 24;
	uint8_t freqFB = (uint8_t) freqNF; // truncate the int
	uint16_t freqFC = (uint16_t) ((int)(freqNF - freqFB) * 64000);

#ifdef DEBUG
	printf("hbsel %d\n", hbsel);
	printf("freqFB %d\n", freqFB);
	printf("freqFC %d\n", freqFC);
#endif

	// sideband is always on (0x40) :
	byte vals[3] = { 0x40 | (hbsel << 5) | (freqFB & 0x3F), freqFC >> 8, freqFC & 0xFF };
	si443x_write(SI443X_REG_FREQBAND, vals, 3);
}

void si443x_set_frequency_mhz(uint16_t freq_mhz) {
	if ((freq_mhz < 240) || (freq_mhz > 930)) {
	//    printf("error: invalid frequency: %lu\n", freqTX);
	return; // invalid frequency
	}

	// set internal variable
	//  _freqCarrier = freqTX;

	char hbsel = (freq >= 480);

	uint16_t freqNF = (freq_mhz / (10 << hbsel)) - 24;
	uint16_t freqFC = (uint16_t) (freq_mhz % (10 << hbsel)) * 64000); // my bad, this is the remainder of division, not result of truncation

	// sideband is always on (0x40) :
	byte vals[3] = { 0x40 | (hbsel << 5) | (freqFB & 0x3F), freqFC >> 8, freqFC & 0xFF };
	si443x_write(SI443X_REG_FREQBAND, vals, 3);
}
