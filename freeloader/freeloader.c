#include <pmb8876_uart.h>
#include <cfi_flash.h>

typedef volatile unsigned int vu32;

#define WDT_CON0	*(vu32 *)0xF4400024
#define WDT_CON1	*(vu32 *)0xF4400028
#define WDT_STAT	*(vu32 *)0xF440002C	// The top 4 bytes are the current wdt
#define STM_CLC		*(vu32 *)0xf4b00000
#define STM_TIM4	*(vu32 *)0xf4b00020

//EBU
#define SDRMREF0	*(vu32 *)0xF0000040
#define SDRMCON0	*(vu32 *)0xF0000050
#define SDRMOD0		*(vu32 *)0xF0000060
#define ADDRSEL0	*(vu32 *)0xF0000080
#define ADDRSEL1	*(vu32 *)0xF0000088
#define ADDRSEL4	*(vu32 *)0xF00000A8
#define BUSCON0		*(vu32 *)0xF00000C0
#define BUSCON1		*(vu32 *)0xF00000C8
#define BUSCON4		*(vu32 *)0xF00000E0

#define HWID1		*(vu32 *)0xF440006C
#define HWID2		*(vu32 *)0xF4400070
#define HWID3		*(vu32 *)0xF4400074

#define NOR		((unsigned char*)0xA0000000)
#define	NORWORD		((unsigned short int*)0xA0000000)
#define MEM		((unsigned char*)0xA8000000)

void
disablewdt()
{
    WDT_CON0 = 0xED6800F1;	//First(password)
    WDT_CON0 = 0xAAAA00F2;      //Unlock endinit(modify)
    WDT_CON1 = 8;		//This is the disable request
    WDT_CON0 = 0xAAAA00F8;      //Relock endinit(pw)
    WDT_CON0 = 0xAAAA00F3;      //Relock endinit(modify)
}

int freeloader_entry() __attribute__ ((section (".rom_vectors")));

int
freeloader_entry()
{
    int i;
    unsigned char waitbyte;

    /* 
     * I don't know if the watchdog is disabled at this point
     * or not, but why take chances
     */
    disablewdt();

    uart_set_speed(0, USART_115200);

    while(1) {
	uart_poll_tx_byte('F');
	for(i = 0; i < 1000000; i++);
    }
}
