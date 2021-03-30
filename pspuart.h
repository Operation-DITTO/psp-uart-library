#ifndef ___SIODRIVER_H___
#define ___SIODRIVER_H___

void    pspUARTInit(int baud);                              // User-executed intitalization function
void    pspUARTTerminate();                                 // Terminate the UART I/O
int     pspUARTAvailable(void);                             // Checks if the ringbuffer is empty, returns > 0 if not
int     pspUARTRead(void);                                  // Returns byte inside of ringbuffer, if empty returns -1
void    pspUARTWrite(int ch);                               // Writes a byte to transmit to the UART chip, should be threaded.
void    pspUARTWriteBuffer(const char *data, int len);      // Writes a buffer 1 character at a time using pspUARTWrite()
void    pspUARTPrint(const char *str);                      // Write buffer and automatically determine length

#endif