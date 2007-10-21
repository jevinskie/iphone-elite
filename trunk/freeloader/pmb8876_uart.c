//#include <pmb8876_reg.h>
//#include <pmb8876_uart.h>

#define			USART0_TXB		*(volatile unsigned int*)0xF1000020
#define			USART0_RXB		*(volatile unsigned int*)0xF1000024
#define			USART0_FCSTAT		*(volatile unsigned int*)0xF1000068
#define			USART0_ICR		*(volatile unsigned int*)0xF1000070

unsigned char
uart_poll_rx_byte()
{
#if 0
    while(!(USART0_FCSTAT & 4));
    
    PMB8876_REG_SET_BIT(USART0_ICR, 4);
    
    return(PMB8876_REG(USART0_RXB) & 0xFF);
#else
	while(	(USART0_FCSTAT & 4) == 0 );

	USART0_ICR |= 4;

	return USART0_RXB&255;
#endif
}

void
uart_poll_tx_byte(unsigned char byte)
{
#if 0
    unsigned int data;

    data = (PMB8876_REG(USART0_TXB) & ~0xFF) | byte;

    PMB8876_REG_SET_MASK(USART0_TXB, data);

    while(!(USART0_FCSTAT & 2));

    PMB8876_REG_SET_BIT(USART0_ICR, 2);
#else
	unsigned int data = USART0_TXB;
	
	data &= ~0xFF;
	data |= byte;

	USART0_TXB = data;

	while(  (USART0_FCSTAT&2) == 0 );

	USART0_ICR	|= 2;
#endif
}

void
uart_poll_tx_string(const char *msg)
{
    while(*msg) {
	uart_poll_tx_byte(*msg);
	msg++;
    }
}
