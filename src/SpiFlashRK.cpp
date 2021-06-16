
#include <Arduino.h>
#include <DSPI.h>
#include <Streaming.h>
#include <sys/kmem.h>
// #include "sys_devcon.h"
// #include "sys_devcon_cache.h"
#include "SpiFlashRK.h"

DSPI0 _flash_spi;
#define _FLASH_SPI_BASE _DSPI0_BASE
#define ASYNC_ENA
#ifdef ASYNC_ENA
volatile uint8_t __attribute__((coherent)) txBuf_g[4];
volatile uint8_t __attribute__((coherent)) rxBuf_g[4];
volatile uint8_t __attribute__((coherent)) txpageBuf_g[PAGE_SIZE + 1];
volatile uint8_t __attribute__((coherent)) rxpageBuf_g[PAGE_SIZE + 1];
#endif
//#define INT_ENA

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
#ifdef ASYNC_ENA
	_flash_spi.beginasync(cs_pin, 4, 5);
	//_flash_spi.begin(cs_pin);
#else
	_flash_spi.begin(cs_pin);
	log_d("init stat %x", _flash_spi.spistat());
#endif
	csSetFast();
    
	setSpiSettings();

	p32_spi *pspi = (p32_spi *) _FLASH_SPI_BASE;
	// pspi->sxCon.clr = (1 << _SPICON_ON);
	// pspi->sxCon.reg = 0;
	// pspi->sxCon.set = ((1 << _SPICON_CKP) | (0 << _SPICON_CKE) | (0 << _SPICON_SMP) | (1 << _SPICON_MSTEN));
	// pspi->sxCon.set = (1 << _SPICON_ON);
	
	//pspi->sxBuf.reg = 0;
	//uint8_t dump = pspi->sxBuf.reg;
	//(void)dump;
	log_d("wake from Sleep %ul\r\n", _flash_spi.spistat());
	// Send release from powerdown 0xab
	wakeFromSleep();
	log_d("BEGIN end\r\n");
}

bool SpiFlash::isValid() {
	uint8_t foundManufacturerId = (jedecIdRead() >> 16) & 0xff;

	return manufacturerId == foundManufacturerId;
}


void SpiFlash::beginTransaction() {
	csResetFast();//noInterrupts();
}

void SpiFlash::endTransaction() {
	csSetFast();//interrupts();
}

void SpiFlash::setSpiSettings() {
	_flash_spi.setTransferSize(DSPI_8BIT);
	_flash_spi.setSpeed(20000000); // Default: 30
	_flash_spi.setMode(DSPI_MODE3); // Default: SPI_MODE3
#ifdef INT_ENA
	_flash_spi.enableInterruptTransfer();
#else
	_flash_spi.disableInterruptTransfer();
#endif
}


uint32_t SpiFlash::jedecIdRead() {
 #ifdef ASYNC_ENA
	txBuf_g[0] = 0x9f;
	txBuf_g[1] = 0x00;
	txBuf_g[2] = 0x00;
	txBuf_g[3] = 0x00;
	rxBuf_g[0] = 0x00;
	rxBuf_g[1] = 0x00;
	rxBuf_g[2] = 0x00;
	rxBuf_g[3] = 0x00;
#else
	uint8_t txBuf[4], rxBuf[4];
	txBuf[0] = 0x9f;
	// uint8_t *ptr;
	// for (ptr = txBuf; ptr<txBuf+4;ptr++){
	// 	_cache(((1)|(5<<2)), ptr);
	// }
	// _sync();

#endif
	beginTransaction();
#ifdef ASYNC_ENA
	//_flash_spi.asyncTransfer(4, (uint8_t*) txBuf, (uint8_t*) rxBuf);
	//do {
	// 	delay(1);
	// 	log_e("waiting %d %d %x %x %x %x\r\n", _flash_spi.transCount(), _flash_spi.isOverflow(), _flash_spi.intflag(), DCH4INT, DCH5INT, DCH4DSIZ);
	//}while (DCH4INTbits.CHBCIF == 0 && DCH5INTbits.CHBCIF == 0);
	_flash_spi.asyncTransfertimeout(4, (uint8_t*)txBuf_g, (uint8_t*)rxBuf_g, 50);
	//_flash_spi.asyncTransfertimeout(4, (uint8_t*) txBuf,(uint8_t*)rxBuf, 50);
	//_flash_spi.asyncTransfertimeout(1, (uint8_t*) txBuf, 50);
	//_flash_spi.asyncTransfertimeout(3, (uint8_t) 0, (uint8_t*) rxBuf, 50);
	//_flash_spi.asyncTransfertimeout(3, (uint8_t*) txBuf, (uint8_t*) rxBuf, 50);
	// for (ptr = rxBuf; ptr<rxBuf+4;ptr++){
	// 	_cache(((1)|(4<<2)), ptr);
	// }
	// _sync();
	//_flash_spi.asyncTransfertimeout(4, (uint8_t *) txBuf_g, (uint8_t *) rxBuf_g, 50);
	//log_e("isoverflow:%d",_flash_spi.isOverflow());
	//_flash_spi.asyncTransfertimeout(4, (uint8_t *) txBuf_g, (uint8_t *) rxBuf_g, 50);
	//_flash_spi.asyncTransfertimeout(1, (uint8_t *) txBuf_g, 50);
	//_flash_spi.asyncTransfertimeout(3, (uint8_t)0x00, (uint8_t *) &rxBuf_g[1], 50);
	// do {
	// 	delay(1);
	// 	log_e("waiting %d %d %x\r\n", _flash_spi.transCount(), _flash_spi.isOverflow(), _flash_spi.intflag());
	// }while (_flash_spi.transCount() != 0);
#elif defined(INT_ENA)
	_flash_spi.intTransfertimeout(sizeof(txBuf), txBuf, rxBuf, 50);
#else
	_flash_spi.transfer(sizeof(txBuf), txBuf, rxBuf);
#endif
	endTransaction();
	//uint8_t * ptr1 = (uint8_t*) KVA0_TO_KVA1(rxBuf);
#ifdef ASYNC_ENA
	//return (rxBuf_g[1] << 16) | (rxBuf_g[2] << 8) | (rxBuf_g[3]);
	//return ((ptr1[1]) << 16) | ((ptr1[2]) << 8) | ((ptr1[3]));
	log_e("JEDEC:%x %x %x %x \r\n", rxBuf_g[0], rxBuf_g[1], rxBuf_g[2], rxBuf_g[3]);
	return (rxBuf_g[1] << 16) | (rxBuf_g[2] << 8) | (rxBuf_g[3] << 0);// | (rxBuf[3]);
#else
	//_cache(((1)|(4<<2)), rxBuf);
	log_e("JEDEC:%x %x %x %x \r\n", rxBuf[0], rxBuf[1], rxBuf[2], rxBuf[3]);
	return (rxBuf[1] << 16) | (rxBuf[2] << 8) | (rxBuf[3]);
#endif
}

uint8_t SpiFlash::readStatus() {
#ifdef ASYNC_ENA
	txBuf_g[0] = 0x05; // RDSR
	txBuf_g[1] = 0;
	rxBuf_g[0] = 0;
	rxBuf_g[1] = 0;
#else
	uint8_t txBuf[2], rxBuf[2];
	txBuf[0] = 0x05;
	txBuf[1] = 0;
	// uint8_t *ptr;
	// for (ptr = txBuf; ptr<txBuf+4;ptr++){
	// 	_cache(((1)|(5<<2)), ptr);
	// }
	// _sync();
#endif
	beginTransaction();
#ifdef ASYNC_ENA
	//_flash_spi.asyncTransfer(1, (uint8_t*) txBuf_g);
	//_flash_spi.asyncTransfertimeout(1, (uint8_t) 0, (uint8_t *) rxBuf_g, 50);
	//_flash_spi.asyncTransfertimeout(1, txBuf, 50);
	_flash_spi.asyncTransfertimeout(2, (uint8_t*) txBuf_g, (uint8_t*) rxBuf_g, 50);
	// _flash_spi.intTransfer(1, txBuf);
	// do {
	// 	delay(1);
	// 	log_e("waiting %d %d %x\r\n", _flash_spi.transCount(), _flash_spi.isOverflow(), _flash_spi.intflag());
	// }while (_flash_spi.transCount() != 0);
	// _flash_spi.intTransfer(1, (uint8_t)0x0, rxBuf);
	// do {
	// 	delay(1);
	// 	log_e("waiting %d %d %x\r\n", _flash_spi.transCount(), _flash_spi.isOverflow(), _flash_spi.intflag());
	// }while (_flash_spi.transCount() != 0);	
	// for (ptr = rxBuf; ptr<rxBuf+4;ptr++){
	// 	_cache(((1)|(4<<2)), ptr);
	// }
	// _sync();
	log_d("Read Status:%x %x\r\n", rxBuf_g[0], rxBuf_g[1]);
#elif defined(INT_ENA)
	_flash_spi.intTransfertimeout(sizeof(txBuf), txBuf, rxBuf, 50);
	log_d("Read Status:%x %x\r\n", rxBuf[0], rxBuf[1]);
#else
	_flash_spi.transfer( sizeof(txBuf), txBuf, rxBuf);
#endif
	endTransaction();

#ifdef ASYNC_ENA
	return rxBuf_g[1];
#else
	return rxBuf[1];
#endif
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
		if (timeout > 10) {
			delay(1);
		}
	}

	log_e("isWriteInProgress=%d time=%u\r\n", isWriteInProgress(), millis() - startTime);
}


void SpiFlash::writeStatus(uint8_t status) {
	waitForWriteComplete();
#ifdef ASYNC_ENA
	txBuf_g[0] = 0x01; // WRSR
	txBuf_g[1] = status;
#else
	uint8_t txBuf[2];
	txBuf[0] = 0x01;
	txBuf[1] = status;
#endif
	// uint8_t *ptr;
	// for (ptr = txBuf; ptr<txBuf+2;ptr++){
	// 	_cache(((1)|(5<<2)), ptr);
	// }
	// _sync();
	beginTransaction();
#ifdef ASYNC_ENA
	_flash_spi.asyncTransfertimeout(2,(uint8_t *)  txBuf_g, (uint8_t*) rxBuf_g, 50);
#elif defined(INT_ENA)
	_flash_spi.intTransfertimeout(sizeof(txBuf), txBuf, 50);
#else

	_flash_spi.transfer(sizeof(txBuf), txBuf);
#endif
	endTransaction();
}

void SpiFlash::readData(size_t addr, void *buf, size_t bufLen) {
	uint8_t *curBuf = (uint8_t *)buf;
	size_t bufLen_c = bufLen;
	while(bufLen > 0) {
		size_t pageOffset = addr % pageSize;
		size_t pageStart = addr - pageOffset;

		size_t count = (pageStart + pageSize) - addr;
		if (count > bufLen) {
			count = bufLen;
		}
		log_d("%d %d %d %d\r\n", count, bufLen, pageOffset, pageStart);
#ifdef ASYNC_ENA
		setInstWithAddr(0x03, addr, (uint8_t *) txBuf_g);
#else
		uint8_t txBuf[4];
		setInstWithAddr(0x03, addr, txBuf); // READ
#endif
		//uint8_t *ptr;
		// for (ptr = txBuf; ptr<txBuf+4;ptr++){
		// 	_cache(((1)|(5<<2)), ptr);
		// }
		// _sync();
		beginTransaction();
#ifdef ASYNC_ENA
		_flash_spi.asyncTransfertimeout(4, (uint8_t *) txBuf_g, (uint8_t *) rxBuf_g, 50);
		//_flash_spi.asyncTransfer(4, (uint8_t *) txBuf_g);
		//do {
		//	delay(1);
		//	log_e("%x\r\n", DCH5INT);
		//}while (DCH5INTbits.CHBCIF == 0);	

		_flash_spi.asyncTransfertimeout(bufLen, (uint8_t) 0x00, (uint8_t*) rxpageBuf_g, 50);
#elif defined(INT_ENA)
		_flash_spi.intTransfertimeout(sizeof(txBuf), txBuf, 50);
		_flash_spi.intTransfertimeout(bufLen, (uint8_t) 0x00, curBuf, 50);
#else
		_flash_spi.transfer(sizeof(txBuf), txBuf);
		_flash_spi.transfer(bufLen, (uint8_t) 0x00, curBuf);
#endif
		endTransaction();
		// for (ptr = curBuf; ptr<curBuf+bufLen; ptr++){
		// 	_cache(((1)|(4<<2)), ptr);
		// }
		// _sync();
#ifdef ASYNC_ENA
		memcpy((void *) curBuf, (void *) rxpageBuf_g, bufLen);
#endif
		addr += count;
		curBuf += count;
		bufLen -= count;
	}
#ifdef ASYNC_ENA
	// uint8_t * ptr;
	// for(ptr = (uint8_t *) buf; ptr < buf+bufLen_c; ptr++){
	// 	_cache(((1)|(4<<2)), ptr);
	// }
	// _sync();
#endif
}


void SpiFlash::setInstWithAddr(uint8_t inst, size_t addr, uint8_t *buf) {
	buf[0] = inst;
	buf[1] = (uint8_t) (addr >> 16);
	buf[2] = (uint8_t) (addr >> 8);
	buf[3] = (uint8_t) addr;
	// uint8_t *ptr;
	// for (ptr = buf; ptr<buf+4;ptr++){
	// 	_cache(((1)|(5<<2)), ptr);
	// }
	// _sync();
}


void SpiFlash::writeData(size_t addr, const void *buf, size_t bufLen) {
	uint8_t *curBuf = (uint8_t *)buf;
	size_t bufLen_c = bufLen;
	waitForWriteComplete();
#ifdef ASYNC_ENA
	// uint8_t * ptr;
	// for(ptr = (uint8_t *) buf; ptr < buf+bufLen_c; ptr++){
	// 	_cache(((1)|(5<<2)), ptr);
	// }
	// _sync();
#endif
	while(bufLen > 0) {
		size_t pageOffset = addr % pageSize;
		size_t pageStart = addr - pageOffset;

		size_t count = (pageStart + pageSize) - addr;
		if (count > bufLen) {
			count = bufLen;
		}
		log_e("writeData addr=%lx pageOffset=%lu pageStart=%lu count=%lu pageSize=%lu\r\n", addr, pageOffset, pageStart, count, pageSize);
		writeEnable();
#ifdef ASYNC_ENA
		setInstWithAddr(0x02, addr, (uint8_t *) txBuf_g);
#else
		uint8_t txBuf[4];
		setInstWithAddr(0x02, addr, txBuf); // PAGE_PROG
#endif
		// uint8_t *ptr;
		// for (ptr = txBuf; ptr<txBuf+4;ptr++){
		// 	_cache(((1)|(5<<2)), ptr);
		// }
		// _sync();
		// for (ptr = curBuf; ptr<curBuf+count; ptr++){
		// 	_cache(((1)|(5<<2)), ptr);
		// }
		// _sync();
#ifdef ASYNC_ENA
		memcpy((void *)txpageBuf_g, (void *)curBuf, count);
#endif
		beginTransaction();
#ifdef ASYNC_ENA
		_flash_spi.asyncTransfertimeout(4, (uint8_t *) txBuf_g, (uint8_t*) rxBuf_g, 50);
		_flash_spi.asyncTransfertimeout(count, (uint8_t*)txpageBuf_g, (uint8_t*) rxpageBuf_g, 50);
#elif defined(INT_ENA)
		_flash_spi.intTransfertimeout(sizeof(txBuf), txBuf, 50);
		_flash_spi.intTransfertimeout(count, curBuf, 50);
#else
		_flash_spi.transfer(sizeof(txBuf), txBuf);
		_flash_spi.transfer(count, curBuf);
#endif
		endTransaction();

		waitForWriteComplete(pageProgramTimeoutMs);

		addr += count;
		curBuf += count;
		bufLen -= count;
	}

}


void SpiFlash::sectorErase(size_t addr) {
	waitForWriteComplete();
//#ifndef ASYNC_ENA
	uint8_t txBuf[4];
//#endif
	log_e("sectorEraseCmd=%02x\r\n", addr);
	writeEnable();
	//
	// ISSI 25LQ080 uses 0x20 or 0xD7
	// Winbond uses 0x20 only, so use that
#ifdef ASYNC_ENA
	setInstWithAddr(0x20, addr, (uint8_t *) txBuf_g);
#else
	setInstWithAddr(0x20, addr, txBuf); // SECTOR_ER
#endif
	// uint8_t *ptr;
	// for (ptr = txBuf; ptr<txBuf+4;ptr++){
	// 	_cache(((1)|(5<<2)), ptr);
	// }
	// _sync();


	beginTransaction();
#ifdef ASYNC_ENA
	_flash_spi.asyncTransfertimeout(4, (uint8_t *) txBuf_g, (uint8_t *)rxBuf_g, 50);
#elif defined(INT_ENA)
	_flash_spi.intTransfertimeout(sizeof(txBuf), txBuf, 50);
#else
	_flash_spi.transfer(sizeof(txBuf), txBuf);
#endif
	endTransaction();

	waitForWriteComplete(sectorEraseTimeoutMs);
}

void SpiFlash::blockErase(size_t addr) {
	waitForWriteComplete();
	writeEnable();
#ifdef ASYNC_ENA
	setInstWithAddr(0xD8, addr, (uint8_t *) txBuf_g);
#else
	uint8_t txBuf[4];
	setInstWithAddr(0xD8, addr, txBuf); // BLOCK_ER
#endif
	
	// uint8_t *ptr;
	// for (ptr = txBuf; ptr<txBuf+4;ptr++){
	// 	_cache(((1)|(5<<2)), ptr);
	// }
	// _sync();
	beginTransaction();
#ifdef ASYNC_ENA
	_flash_spi.asyncTransfertimeout(4, (uint8_t *) txBuf_g, (uint8_t *)rxBuf_g, 50);
#elif defined(INT_ENA)
	_flash_spi.intTransfertimeout(sizeof(txBuf), txBuf, 50);
#else
	_flash_spi.transfer(sizeof(txBuf), txBuf);
#endif
	endTransaction();

	waitForWriteComplete(chipEraseTimeoutMs);

}

void SpiFlash::chipErase() {
	waitForWriteComplete();
	writeEnable();
	// uint8_t *ptr;
	// for (ptr = txBuf; ptr<txBuf+1;ptr++){
	// 	_cache(((1)|(5<<2)), ptr);
	// }
	// _sync();
#ifdef ASYNC_ENA
	txBuf_g[0] = 0xC7;
#else
	uint8_t txBuf[1];
	txBuf[0] = 0xC7; // CHIP_ER
#endif
	beginTransaction();
#ifdef ASYNC_ENA
	_flash_spi.asyncTransfertimeout(txBuf_g[0], 50);
#elif defined(INT_ENA)
	_flash_spi.intTransfertimeout(sizeof(txBuf), txBuf, 50);
#else
	_flash_spi.transfer(sizeof(txBuf), txBuf);
#endif
	endTransaction();

	waitForWriteComplete(chipEraseTimeoutMs);
}

void SpiFlash::resetDevice() {
	waitForWriteComplete();
#ifdef ASYNC_ENA
	txBuf_g[0] = 0x66;
#else
	uint8_t txBuf[1];
	txBuf[0] = 0x66; // Enable reset
#endif
	// uint8_t *ptr;
	// for (ptr = txBuf; ptr<txBuf+1;ptr++){
	// 	_cache(((1)|(5<<2)), ptr);
	// }
	// _sync();
	beginTransaction();
#ifdef ASYNC_ENA
	_flash_spi.asyncTransfertimeout(txBuf_g[0], 50);
#elif defined(INT_ENA)
	_flash_spi.intTransfertimeout(sizeof(txBuf), txBuf, 50);
#else
	_flash_spi.transfer(sizeof(txBuf), txBuf);
#endif
	endTransaction();

	delayMicroseconds(1);
#ifdef ASYNC_ENA
	txBuf_g[0] = 0x99;
#else
	txBuf[0] = 0x99; // Reset
#endif
	//uint8_t *ptr;
	// for (ptr = txBuf; ptr<txBuf+1;ptr++){
	// 	_cache(((1)|(5<<2)), ptr);
	// }
	// _sync();
	beginTransaction();
#ifdef ASYNC_ENA
	_flash_spi.asyncTransfertimeout(txBuf_g[0], 50);
#elif defined(INT_ENA)
	_flash_spi.intTransfertimeout(sizeof(txBuf), txBuf, 50);
#else
	_flash_spi.transfer(sizeof(txBuf), txBuf);
#endif
	endTransaction();

	delayMicroseconds(1);
}

void SpiFlash::wakeFromSleep() {
	// Send release from powerdown 0xab
#ifdef ASYNC_ENA
	txBuf_g[0] = 0xab;
#else
	uint8_t txBuf[1];
	txBuf[0] = 0xab;
#endif
	// uint8_t *ptr;
	// for (ptr = txBuf; ptr<txBuf+1;ptr++){
	// 	_cache(((1)|(5<<2)), ptr);
	// }
	// _sync();
	beginTransaction();
	//log_e("%ul", _flash_spi.spistat());
#ifdef ASYNC_ENA
	_flash_spi.asyncTransfertimeout(txBuf_g[0], 50);
#elif defined(INT_ENA)
	// log_d("BEGIN INTERRUPT\r\n");
	// _flash_spi.intTransfer(1, txBuf);
	// while (_flash_spi.transCount() != 0){
	// 	log_d("%d", _flash_spi.isOverflow());
	// 	delay(1);
	// }
	_flash_spi.intTransfertimeout(1, txBuf, 50);
#else
	_flash_spi.transfer(1, &txBuf);
#endif
	endTransaction();

	// Need to wait tres (3 microseconds) before issuing the next command
	delayMicroseconds(3);
}

// Note: not all chips support this. Macronix does.
void SpiFlash::deepPowerDown() {
#ifdef ASYNC_ENA
	txBuf_g[0] = 0xb9;
#else
	uint8_t txBuf[1];
	txBuf[0] = 0xb9;
#endif
	// uint8_t *ptr;
	// for (ptr = txBuf; ptr<txBuf+1;ptr++){
	// 	_cache(((1)|(5<<2)), ptr);
	// }
	// _sync();
	beginTransaction();
#ifdef ASYNC_ENA
	_flash_spi.asyncTransfertimeout(txBuf_g[0], 50);
#elif defined(INT_ENA)
	_flash_spi.intTransfertimeout(sizeof(txBuf), txBuf, 50);
#else
	_flash_spi.transfer(sizeof(txBuf), txBuf);
#endif
	endTransaction();

	// Need to wait tdp (10 microseconds) before issuing the next command, but since we're probably doing
	// this before sleep, it's not necessary
}


void SpiFlash::writeEnable() {
#ifdef ASYNC_ENA
	txBuf_g[0] = 0x06;
#else
	uint8_t txBuf[1];
	txBuf[0] = 0x06; //WREN
#endif
	// uint8_t *ptr;
	// for (ptr = txBuf; ptr<txBuf+1;ptr++){
	// 	_cache(((1)|(5<<2)), ptr);
	// }
	// _sync();
	beginTransaction();
#ifdef ASYNC_ENA
	_flash_spi.asyncTransfertimeout(txBuf_g[0], 50);
#elif defined(INT_ENA)
	_flash_spi.intTransfertimeout(sizeof(txBuf), txBuf, 50);
#else
	_flash_spi.transfer(sizeof(txBuf), txBuf);
#endif
	endTransaction();

	// ISSI devices require a 3us delay here, but Winbond devices do not
	if (writeEnableDelayUs > 0) {
		delayMicroseconds(writeEnableDelayUs);
	}
}


