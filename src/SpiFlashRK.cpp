
#include <Arduino.h>
#include <DSPI.h>
#include "SpiFlashRK.h"

DSPI0 _flash_spi;

void SpiFlash::initcs(){
	pinMode(cs_pin, OUTPUT);
}
void SpiFlash::csResetFast(){
	digitalWrite(cs_pin, LOW);
}
void SpiFlash::csSetFast(){
	digitalWrite(cs_pin, HIGH);
}
void SpiFlash::pinResetFast(uint8_t cs_pin){
	digitalWrite(cs_pin, LOW);
}
void SpiFlash::pinSetFast(uint8_t cs_pin){
	digitalWrite(cs_pin, HIGH);
}

SpiFlash::SpiFlash(int cs){
	cs_pin = cs;
	initcs();
}

SpiFlash::~SpiFlash() {

}

void SpiFlash::begin() {
	_flash_spi.begin(cs_pin);
	csSetFast();
    
	setSpiSettings();

	// Send release from powerdown 0xab
	wakeFromSleep();
}

bool SpiFlash::isValid() {
	uint8_t foundManufacturerId = (jedecIdRead() >> 16) & 0xff;

	return manufacturerId == foundManufacturerId;
}


void SpiFlash::beginTransaction() {
	csResetFast();noInterrupts();
}

void SpiFlash::endTransaction() {
	csSetFast();interrupts();
}

void SpiFlash::setSpiSettings() {
	_flash_spi.disableInterruptTransfer();
	_flash_spi.setTransferSize(DSPI_8BIT);
	_flash_spi.setSpeed(20000000); // Default: 30
	_flash_spi.setMode(DSPI_MODE3); // Default: SPI_MODE3
}


uint32_t SpiFlash::jedecIdRead() {

	uint8_t txBuf[4], rxBuf[4];
	txBuf[0] = 0x9f;
	beginTransaction();
	_flash_spi.transfer(sizeof(txBuf), txBuf, rxBuf);
	endTransaction();
	return (rxBuf[1] << 16) | (rxBuf[2] << 8) | (rxBuf[3]);
}

uint8_t SpiFlash::readStatus() {
	uint8_t txBuf[2], rxBuf[2];
	txBuf[0] = 0x05; // RDSR
	txBuf[1] = 0;

	beginTransaction();
	_flash_spi.transfer( sizeof(txBuf), txBuf, rxBuf);
	endTransaction();

	return rxBuf[1];
}

bool SpiFlash::isWriteInProgress() {
	return (readStatus() & STATUS_WIP) != 0;
}

void SpiFlash::waitForWriteComplete(unsigned long timeout) {
	unsigned long startTime = millis();

	if (timeout == 0) {
		timeout = waitWriteCompletionTimeoutMs;
	}

	// Wait for up to 500 ms. Most operations should take much less than that.
	while(isWriteInProgress() && millis() - startTime < timeout) {
		// For long timeouts, yield the CPU
		if (timeout > 500) {
			delay(1);
		}
	}

	log_e("isWriteInProgress=%d time=%u", isWriteInProgress(), millis() - startTime);
}


void SpiFlash::writeStatus(uint8_t status) {
	waitForWriteComplete();

	uint8_t txBuf[2];
	txBuf[0] = 0x01; // WRSR
	txBuf[1] = status;

	beginTransaction();
	_flash_spi.transfer(sizeof(txBuf), txBuf);
	endTransaction();
}

void SpiFlash::readData(size_t addr, void *buf, size_t bufLen) {
	uint8_t *curBuf = (uint8_t *)buf;

	while(bufLen > 0) {
		size_t pageOffset = addr % pageSize;
		size_t pageStart = addr - pageOffset;

		size_t count = (pageStart + pageSize) - addr;
		if (count > bufLen) {
			count = bufLen;
		}

		uint8_t txBuf[4];

		setInstWithAddr(0x03, addr, txBuf); // READ

		beginTransaction();
		_flash_spi.transfer(sizeof(txBuf), txBuf);
		_flash_spi.transfer(bufLen, (uint8_t) 0x00, curBuf);
		endTransaction();

		addr += count;
		curBuf += count;
		bufLen -= count;
	}
}


void SpiFlash::setInstWithAddr(uint8_t inst, size_t addr, uint8_t *buf) {
	buf[0] = inst;
	buf[1] = (uint8_t) (addr >> 16);
	buf[2] = (uint8_t) (addr >> 8);
	buf[3] = (uint8_t) addr;
}


void SpiFlash::writeData(size_t addr, const void *buf, size_t bufLen) {
	uint8_t *curBuf = (uint8_t *)buf;

	waitForWriteComplete();

	while(bufLen > 0) {
		size_t pageOffset = addr % pageSize;
		size_t pageStart = addr - pageOffset;

		size_t count = (pageStart + pageSize) - addr;
		if (count > bufLen) {
			count = bufLen;
		}

		log_e("writeData addr=%lx pageOffset=%lu pageStart=%lu count=%lu pageSize=%lu", addr, pageOffset, pageStart, count, pageSize);

		uint8_t txBuf[4];

		setInstWithAddr(0x02, addr, txBuf); // PAGE_PROG

		writeEnable();

		beginTransaction();
		_flash_spi.transfer(sizeof(txBuf), txBuf);
		_flash_spi.transfer(count, curBuf);
		endTransaction();

		waitForWriteComplete(pageProgramTimeoutMs);

		addr += count;
		curBuf += count;
		bufLen -= count;
	}

}


void SpiFlash::sectorErase(size_t addr) {
	waitForWriteComplete();

	uint8_t txBuf[4];

	log_e("sectorEraseCmd=%02x", addr);

	//
	// ISSI 25LQ080 uses 0x20 or 0xD7
	// Winbond uses 0x20 only, so use that
	setInstWithAddr(0x20, addr, txBuf); // SECTOR_ER


	writeEnable();

	beginTransaction();
	_flash_spi.transfer(sizeof(txBuf), txBuf);
	endTransaction();

	waitForWriteComplete(sectorEraseTimeoutMs);
}

void SpiFlash::blockErase(size_t addr) {
	waitForWriteComplete();

	uint8_t txBuf[4];

	setInstWithAddr(0xD8, addr, txBuf); // BLOCK_ER

	writeEnable();

	beginTransaction();
	_flash_spi.transfer(sizeof(txBuf), txBuf);
	endTransaction();

	waitForWriteComplete(chipEraseTimeoutMs);

}

void SpiFlash::chipErase() {
	waitForWriteComplete();

	uint8_t txBuf[1];

	txBuf[0] = 0xC7; // CHIP_ER

	writeEnable();

	beginTransaction();
	_flash_spi.transfer(sizeof(txBuf), txBuf);
	endTransaction();

	waitForWriteComplete(chipEraseTimeoutMs);
}

void SpiFlash::resetDevice() {
	waitForWriteComplete();

	uint8_t txBuf[1];

	txBuf[0] = 0x66; // Enable reset

	beginTransaction();
	_flash_spi.transfer(sizeof(txBuf),txBuf);
	endTransaction();

	delayMicroseconds(1);

	txBuf[0] = 0x99; // Reset

	beginTransaction();
	_flash_spi.transfer(sizeof(txBuf),txBuf);
	endTransaction();

	delayMicroseconds(1);
}

void SpiFlash::wakeFromSleep() {
	// Send release from powerdown 0xab
	uint8_t txBuf[1];
	txBuf[0] = 0xab;

	beginTransaction();
	_flash_spi.transfer(sizeof(txBuf),txBuf);
	endTransaction();

	// Need to wait tres (3 microseconds) before issuing the next command
	delayMicroseconds(3);
}

// Note: not all chips support this. Macronix does.
void SpiFlash::deepPowerDown() {

	uint8_t txBuf[1];
	txBuf[0] = 0xb9;

	beginTransaction();
	_flash_spi.transfer(sizeof(txBuf), txBuf);
	endTransaction();

	// Need to wait tdp (10 microseconds) before issuing the next command, but since we're probably doing
	// this before sleep, it's not necessary
}


void SpiFlash::writeEnable() {
	uint8_t txBuf[1];

	beginTransaction();
	txBuf[0] = 0x06; // WREN
	_flash_spi.transfer(sizeof(txBuf), txBuf);
	endTransaction();

	// ISSI devices require a 3us delay here, but Winbond devices do not
	if (writeEnableDelayUs > 0) {
		delayMicroseconds(writeEnableDelayUs);
	}
}


