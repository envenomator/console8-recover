#include "zdi.h"
#include "cpu.h"
#include "flashloader.h"
#include "eZ80F92.h"
#include <esp_task_wdt.h>
#include "esp32_io.h"

#define PAGESIZE       1024
#define USERLOAD    0x40000
#define BREAKPOINT  0x40020

#define LEDFLASHFASTMS     75
#define LEDFLASHSTEADYMS   500
#define LEDFLASHWAIT       300

#define WAITHOLDBUTTONMS  2000
#define WAITRESETMS       5000

#define ZDI_TCKPIN 26
#define ZDI_TDIPIN 27

int ledpins[] = {2,4,16}; // should cover most esp32 boards, including ezsbc board
int ledpins_onstate[] = {1,1,0}; // 1: Pin high == on, 0: Pin low == on

unsigned long time0;
bool btn_pressed;

CPU*                    cpu;
ZDI*                    zdi;

void setupLedPins(void) {
    for(int n = 0; n < (sizeof(ledpins) / sizeof(int)); n++)
        directModeOutput(ledpins[n]);
}

void ledsOff(void) {
    for(int n = 0; n < (sizeof(ledpins) / sizeof(int)); n++)
        if(ledpins_onstate[n]) directWriteLow(ledpins[n]);
        else directWriteHigh(ledpins[n]);
}

void ledsOn(void) {
    for(int n = 0; n < (sizeof(ledpins) / sizeof(int)); n++)
        if(ledpins_onstate[n]) directWriteHigh(ledpins[n]);
        else directWriteLow(ledpins[n]);
}

void ledsFlash(void) {
    ledsOff();
    delay(LEDFLASHFASTMS);
    ledsOn();
    delay(LEDFLASHFASTMS);
}

void ledsErrorFlash(void) {
    int n;
    for(n = 0; n < 4; n++) ledsFlash();
    delay(LEDFLASHWAIT);
}

void ledsWaitFlash(int32_t ms) {
    while(ms > 0) {
        ledsOff();
        delay(LEDFLASHSTEADYMS);
        ledsOn();
        delay(LEDFLASHSTEADYMS);
        ms -= 2*LEDFLASHSTEADYMS;
    }
}

void setup() {
    // Disable the watchdog timers
    disableCore0WDT(); delay(200);								
    esp_task_wdt_init(30, false); // in case WDT cannot be removed

    // setup ZDI interface
    zdi = new ZDI(ZDI_TCKPIN, ZDI_TDIPIN);
    cpu = new CPU(zdi);

    // setup LED pins
    setupLedPins();

    // configure boot button as input
    directModeInput(0);

    // Start timer
    time0 = millis();
    btn_pressed = false;
}

void init_ez80(void) {
    cpu->setBreak();
    cpu->setADLmode(true);
    cpu->instruction_di();  
    
    // configure SPI
    cpu->instruction_out (SPI_CTL, 0x04);
    // configure default GPIO
    cpu->instruction_out (PB_DDR, 0xff);
    cpu->instruction_out (PC_DDR, 0xff);
    cpu->instruction_out (PD_DDR, 0xff);
    
    cpu->instruction_out (PB_ALT1, 0x0);
    cpu->instruction_out (PC_ALT1, 0x0);
    cpu->instruction_out (PD_ALT1, 0x0);
    cpu->instruction_out (PB_ALT2, 0x0);
    cpu->instruction_out (PC_ALT2, 0x0);
    cpu->instruction_out (PD_ALT2, 0x0);

    cpu->instruction_out (TMR0_CTL, 0x0);
    cpu->instruction_out (TMR1_CTL, 0x0);
    cpu->instruction_out (TMR2_CTL, 0x0);
    cpu->instruction_out (TMR3_CTL, 0x0);
    cpu->instruction_out (TMR4_CTL, 0x0);
    cpu->instruction_out (TMR5_CTL, 0x0);

    cpu->instruction_out (UART0_IER, 0x0);
    cpu->instruction_out (UART1_IER, 0x0);

    cpu->instruction_out (I2C_CTL, 0x0);
    cpu->instruction_out (FLASH_IRQ, 0x0);

    cpu->instruction_out (SPI_CTL, 0x4);

    cpu->instruction_out (RTC_CTRL, 0x0);
    
    // configure internal flash
    cpu->instruction_out (FLASH_ADDR_U,0x00);
    cpu->instruction_out (FLASH_CTRL,0b00101000);   // flash enabled, 1 wait state
    
    // configure internal RAM chip-select range
    cpu->instruction_out (RAM_ADDR_U,0xb7);         // configure internal RAM chip-select range
    cpu->instruction_out (RAM_CTL,0b10000000);      // enable
    // configure external RAM chip-select range
    cpu->instruction_out (CS0_LBR,0x04);            // lower boundary
    cpu->instruction_out (CS0_UBR,0x0b);            // upper boundary
    cpu->instruction_out (CS0_BMC,0b00000001);      // 1 wait-state, ez80 mode
    cpu->instruction_out (CS0_CTL,0b00001000);      // memory chip select, cs0 enabled

    // configure external RAM chip-select range
    cpu->instruction_out (CS1_CTL,0x00);            // memory chip select, cs1 disabled
    // configure external RAM chip-select range
    cpu->instruction_out (CS2_CTL,0x00);            // memory chip select, cs2 disabled
    // configure external RAM chip-select range
    cpu->instruction_out (CS3_CTL,0x00);            // memory chip select, cs3 disabled

    // set stack pointer
    cpu->sp(0x0BFFFF);
    // set program counter
    cpu->pc(0x000000);
}

void ZDI_upload(void) {
    uint32_t address,filesize;
    unsigned char *buffer;

    address = USERLOAD;
    buffer = &flashloader_bin[0];
    filesize = flashloader_bin_len;

    while(filesize > PAGESIZE) {
        zdi->write_memory(address, PAGESIZE, buffer);
        address += PAGESIZE;
        buffer += PAGESIZE;
        filesize -= PAGESIZE;
        ledsFlash();
    }
    zdi->write_memory(address, filesize, buffer);
    ledsFlash();
}

void flashMOS(void) {
    init_ez80();
    ledsFlash();
    ZDI_upload();
    ledsFlash();

    cpu->pc(USERLOAD);
    cpu->setContinue(); // start flashloader, no feedback
    
    ledsFlash();
    ledsWaitFlash(WAITRESETMS);
}

void loop() {

    if((zdi->get_productid() != 7)) {
        ledsErrorFlash();
        ledsOff();
        return;
    }

    ledsOn();

    if(directRead(0) == 0) btn_pressed = true;
    else {
        btn_pressed = false;
        time0 = millis();
    }

    if(btn_pressed && (millis() - time0) > WAITHOLDBUTTONMS) flashMOS();

}
