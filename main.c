//
// psp-uart-library 
// (c) 2016 DavisDev
// (c) 2021 OP-DITTO Team
// 
// psp-uart-library is licensed under a
// Creative Commons Attribution-ShareAlike 4.0 International License.
// 
// You should have received a copy of the license along with this
// work. If not, see <http://creativecommons.org/licenses/by-sa/4.0/>.
//
// psp-uart-library is a fork of SioDriver by DavisDev, the original
// repository can be found here:
// https://github.com/DavisDev/SioDriver
//
// Original additional credits for SioDriver:
// Tyranid's - Sio based on psplink
// forum.ps2dev.org - More info over that port
// Jean - Initial driver sio
//

#include <pspsdk.h>
#include <pspintrman_kernel.h>
#include <pspintrman.h>
#include <pspsyscon.h>

//
// Module info - remove these if embedding into application!
//
PSP_MODULE_INFO("pspUART", 0x1006, 1, 1);
PSP_NO_CREATE_MAIN_THREAD(); 

//
// Memory addresses for the UART controller
//
#define PSP_UART4_DIV1 				0xBE500024
#define PSP_UART4_DIV2 				0xBE500028
#define PSP_UART4_CTRL 				0xBE50002C
#define PSP_UART4_FIFO 				0xBE500000
#define PSP_UART4_STAT 				0xBE500018

#define PSP_UART_INTR_ADDR			0xBE500040
#define PSP_UART_INTR_CLR_ADDR 		0xBE500044

#define PSP_UART_TXFULL  			0x20
#define PSP_UART_RXEMPTY 			0x10
#define PSP_UART_CLK   				96000000

#define SIO_CHAR_RECV_EVENT  		0x01
#define SERIAL_BUFFER_SIZE 			64				// Unlikely to require a larger buffer, PSP is limited to 4800 bytes per second anyway

static SceUID sio_eventflag 		= -1;

//
// Stubs for sio.c
// 
void 	sceHprmEnd(void);
void 	sceHprmReset(void);
void 	sceHprmInit(void);
void 	sceSysregUartIoEnable(int);

//
// Ring Buffer struct
//
typedef struct
{
	unsigned char 			buffer[SERIAL_BUFFER_SIZE];		// Transmitted data
	volatile unsigned int 	head;							// The HEAD of the ringbuffer
	volatile unsigned int 	tail;							// The TAIL of the ringbuffer
}

// Create the dummy ring buffer
ringbuffer_t rx_buffer = { { 0 }, 0, 0 };

//
// store_char(c, buffer)
// Insert data into the ringbuffer
//
void store_char(unsigned char c, ringbuffer_t *buffer)
{
	// helps prevent writing into the tail and causing a buffer overflow
	int i = (unsigned int)(buffer->head + 1) % SERIAL_BUFFER_SIZE;

	if (i != buffer->tail) {
		buffer->buffer[buffer->head] = c;
		buffer->head = i;
	}
}

//
// intr_handler(arg)
// Callback thread for the SIO
//
int intr_handler(void *arg)
{
	// Disable interupts to avoid complication
	sceKernelDisableIntr(PSP_HPREMOTE_INT);

	// Clear the interupt state
	u32 stat = _lw(PSP_UART_INTR_ADDR);
	_sw(stat, PSP_UART_INTR_CLR_ADDR);

	// Our transmitter is not empty, store into the ring buffer and set our event flag
	if (!(_lw(PSP_UART4_STAT & PSP_UART_RXEMPTY))) {
		store_char(_lw(PSP_UART4_FIFO), &rx_buffer);
		sceKernelSetEventFlag(sio_eventflag, SIO_CHAR_RECV_EVENT);
	}

	// Enable interupts again
	sceKernelEnableIntr(PSP_HPREMOTE_INT);

	return -1;
}

//
// _pspUARTInit()
// Handles enabling the UART transmitter and sets up HPRM
//
void _pspUARTInit(void)
{
	sceHprmReset();
	sceHprmEnd();
	sceSysregUartIoEnable(4);
	sceSysregUartIoEnable(1);
}

//
// pspUARTSetBaud(baud)
// Sets the BUAD (bytes per second) for the transmitter
//
void pspUARTSetBaud(int baud)
{
	// low, high bits
	int div1, div2;

	div1 = PSP_UART_CLK / baud;
	div2 = div1 & 0x3F;
	div1 >>= 6;

	_sw(div1, PSP_UART4_DIV1);
	_sw(div2, PSP_UART4_DIV2);
	_sw(0x60, PSP_UART4_CTRL);
}

//
// pspUARTInit(baud)
// User-executed intitalization function
//
void pspUARTInit(int baud)
{
	unsigned int k1 = pspSdkSetK1(0);
	_pspUARTInit();
	sio_eventflag = sceKernelCreateEventFlag("SioShellEvent", 0, 0, 0);

	sceKernelRegisterIntrHandler(PSP_HPREMOTE_INT, 1, intr_handler, NULL, NULL);
	sceKernelEnableIntr(PSP_HPREMOTE_INT);
	sceKernelDelayThread(2000000);
	pspUARTSetBaud(baud);
	pspSdkSetK1(k1); 
}

//
// pspUARTTerminate
// Terminate the UART I/O
//
void pspUARTTerminate()
{
	unsigned int k1 = pspSdkSetK1(0);
	sceSysconCtrlHRPower(0);
	sceKernelDeleteEventFlag(sio_eventflag);
	sceKernelReleaseIntrHandler(PSP_HPREMOTE_INT);
	sceKernelDisableIntr(PSP_HPREMOTE_INT);
	sceHprmReset();
	sceHprmInit();
	pspSdkSetK1(k1);
}

//
// pspUARTAvailable
// Checks if the ringbuffer is empty, returns > 0 if not
//
int pspUARTAvailable(void)
{
	return (unsigned int)(SERIAL_BUFFER_SIZE + rx_buffer.head - rx_buffer.tail) % SERIAL_BUFFER_SIZE;
}

//
// pspUARTRead
// Returns char inside of ringbuffer, if not empty
//
int pspUARTRead(void)
{
	// empty buffer
	if (rx_buffer.head == rx_buffer.tail) {
		return -1;
	} else {
		unsigned char c = rx_buffer.buffer[rx_buffer.tail];
		rx_buffer.tail = (unsigned int)(rx_buffer.tail + 1) % SERIAL_BUFFER_SIZE;
		return c;
	}
}

//
// pspUARTWrite(ch)
// Writes a character to transmit to the UART chip,
// should be threaded.
//
void pspUARTWrite(int ch)
{
	while(_lw(PSP_UART4_STAT) & PSP_UART_TXFULL);
	_sw(ch, PSP_UART4_FIFO);
}

//
// pspUARTWriteBuffer(data, len)
// Writes a buffer 1 character at a time using pspUARTWrite()
//
void pspUARTWriteBuffer(const char *data, int len)
{
	unsigned int k1 = pspSdkSetK1(0);

	for (int i = 0; i < len; i++) {
		pspUARTWrite(data[i]);
	}

	pspSdkSetK1(k1);
}

//
// pspUARTPrint(str)
// Write buffer and automatically determine length
//
void pspUARTPrint(const char *str)
{
	unsigned int k1 = pspSdkSetK1(0); 

	while (*str){
		pspUARTWrite(*str);
		str++;
	}

	pspSdkSetK1(k1); 
}

//
// Module info stubs - remove these if embedding into application!
//
int module_start(SceSize args, void *argp){ return 0; }
int module_stop() { return 0; }
