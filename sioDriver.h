#ifndef ___SIODRIVER_H___
#define ___SIODRIVER_H___

// Sio function prototypes

void sioInit(int baud); // Initialize Sio Port
void sioFinalize(); // Finalize Sio Port
int sioAvailable(void); // Check if there is something in buffer
int sioRead(void); // Reads a byte of SIO port, if nothing returns -1
void sioWrite(int ch); // Write a byte of the SIO port
void sioWriteBuff(const char *data, int len); // Write a buffer to the SIO port, the length of the buffer is indicated
void sioPrint(const char *str); // Write a buffer to the port SIO


#endif
