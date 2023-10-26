#include <ez80.h>
#include "src_fatfs\ff.h"
#include "spi.h"
#include "defines.h"
#include <stdint.h>
#include "agontimer.h"

#define PAGESIZE	1024
#define FLASHPAGES	128
#define FLASHSTART	0x0

#define MOSFILENAME	"MOS.bin"
#define LOADADDRESS	0x50000

extern void enableFlashKeyRegister(void);
extern void lockFlashKeyRegister(void);
extern void fastmemcpy(UINT24 destination, UINT24 source, UINT24 size);
extern void reset(void);

FATFS 	fs;

int main(int argc, char * argv[]) {
	FRESULT	frmos;
	FIL	   	filmos;
	UINT24  br;	
	UINT24  mossize;
	
	UINT24	counter,pagemax, lastpagebytes;
	UINT24 addressto,addressfrom;
	UINT8	value;

	init_spi();

	f_mount(&fs, "", 1);

	addressto = FLASHSTART;
	addressfrom = LOADADDRESS;

	frmos= f_open(&filmos, MOSFILENAME, FA_READ);
	if(frmos != FR_OK) while(1);
	mossize = f_size(&filmos);

	frmos= f_read(&filmos, (void *)LOADADDRESS, mossize, &br);

	f_close(&filmos);

	// Unprotect and erase flash
	enableFlashKeyRegister();	// unlock Flash Key Register, so we can write to the Flash Write/Erase protection registers
	FLASH_PROT = 0;				// disable protection on all 8x16KB blocks in the flash
	enableFlashKeyRegister();	// will need to unlock again after previous write to the flash protection register
	FLASH_FDIV = 0x5F;			// Ceiling(18Mhz * 5,1us) = 95, or 0x5F

	// Mass erase flash
	FLASH_PGCTL = 0x01;	// mass erase bit enable, start erase all pages
	do {
		value = FLASH_PGCTL;
	} while(value & 0x01); // wait for completion of erase

	// determine number of pages to write
	pagemax = mossize/PAGESIZE;
	if(mossize%PAGESIZE) // last page has less than PAGESIZE bytes
	{
		pagemax += 1;
		lastpagebytes = mossize%PAGESIZE;			
	}
	else lastpagebytes = PAGESIZE; // normal last page

	// write out each page to flash
	for(counter = 0; counter < pagemax; counter++)
	{
		if(counter == (pagemax - 1)) // last page to write - might need to write less than PAGESIZE
			fastmemcpy(addressto,addressfrom,lastpagebytes);				
		else 
			fastmemcpy(addressto,addressfrom,PAGESIZE);
	
		addressto += PAGESIZE;
		addressfrom += PAGESIZE;
	}
	lockFlashKeyRegister();	// lock the flash before WARM reset

	reset();
	return 0;
}
