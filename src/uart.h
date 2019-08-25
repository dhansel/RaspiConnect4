#ifndef UART_H
#define UART_H

#ifdef __cplusplus 
extern "C"         
{                  
#endif

extern void uart_init(void);
extern unsigned int uart_poll();
extern void uart_purge();
extern void uart_write(const char* data, unsigned int size );
extern void uart_write_str(const char* data);
extern unsigned int uart_read_byte();

#ifdef __cplusplus 
}
#endif


#endif
