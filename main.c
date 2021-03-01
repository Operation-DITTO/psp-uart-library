/*
	SIO DRIVER - "Serial Conductor"
	Driver for use "sio" over user app

	Developed by Davis Nuñez - David.nunezaguilera.131@gmail.com
	
	Licensed by Creative Commons Attribution-Share 4.0
		https://creativecommons.org/licenses/by-sa/4.0/

	Version: 
		4.0 Full - 13/01/2016 - 13:34 pm
		Added an intelligent buffer ring, and re written functions.
	Thanks to:
	Tyranid's - Sio based on pspLink.
	forum.ps2dev.org - More info over that port.
	Jean - Initial Driver sio.
	
	Do whatever you want with this...if you want, give credit to the original authors, and put your name in this list if you enhance it :)
*/

#include <pspsdk.h>
#include <pspintrman_kernel.h>
#include <pspintrman.h>
#include <pspsyscon.h>

PSP_MODULE_INFO("sioDriverv4", 0x1006, 1, 1);
PSP_NO_CREATE_MAIN_THREAD(); 

// no changes here...the same good old consts
#define PSP_UART4_DIV1 				0xBE500024
#define PSP_UART4_DIV2 				0xBE500028
#define PSP_UART4_CTRL 				0xBE50002C
#define PSP_UART4_FIFO 				0xBE500000
#define PSP_UART4_STAT 				0xBE500018

#define SIO_CHAR_RECV_EVENT  		0x01

#define PSP_UART_TXFULL  			0x20
#define PSP_UART_RXEMPTY 			0x10
#define PSP_UART_CLK   				96000000

// Reception Functions :D
#define SERIAL_BUFFER_SIZE 			1024 // More space for data

static SceUID sio_eventflag 		= -1;

// imported power functions prototypes
void 	sceHprmEnd(void);
void 	sceHprmReset(void);
void 	sceHprmInit(void);

void 	sceSysregUartIoEnable(int);

// prototypes for stubs
int 	sceCodecOutputEnable(int, int); // enable_hearphone, enable_speaker
int 	sceHprmSetConnectCallback(int); // for parameters...just a guess: have to look into disassemblies.
int 	sceHprmRegisterCallback(int);

typedef struct { // Smart ring buffer
	unsigned char 			buffer[SERIAL_BUFFER_SIZE];
	volatile unsigned int 	head;
	volatile unsigned int 	tail;
} ringbuffer_t;

ringbuffer_t rx_buffer = { { 0 }, 0, 0}; // We create the rx buffer, and initial config ;)

void store_char(unsigned char c, ringbuffer_t *buffer){ // Function that inserts an entry in the ring
	int i = (unsigned int)(buffer->head + 1) % SERIAL_BUFFER_SIZE;
	// if we should be storing the received character into the location
	// just before the tail (meaning that the head would advance to the
	// current location of the tail), we're about to overflow the buffer
	// and so we don't write the character or advance the head.
	if(i != buffer->tail){
		buffer->buffer[buffer->head] = c;
		buffer->head = i;
	}
}

int intr_handler(void *arg){ // Call it the callback or thread of recv ;)
	// disable interrupt...we don't want SIO to call intr_handler again while it's already running
	// don't know if it's really needed here, but i remember this was a must in pc programming
	// MAYBE i'm better use "int intrs = pspSdkDisableInterrupts();" (disable ALL intrs) to handle reader/writer conflicts

	sceKernelDisableIntr(PSP_HPREMOTE_INT); 

	/* Read out the interrupt state and clear it */
	u32 stat = _lw(0xBE500040);
	_sw(stat, 0xBE500044);

	if(!(_lw(PSP_UART4_STAT) & PSP_UART_RXEMPTY)) {
		store_char(_lw(PSP_UART4_FIFO), &rx_buffer);
		sceKernelSetEventFlag(sio_eventflag, SIO_CHAR_RECV_EVENT); // set "we got something!!" flag
	}

	sceKernelEnableIntr(PSP_HPREMOTE_INT); // re-enable interrupt
	// MAYBE i'm better use "pspSdkEnableInterrupts(intrs);"

	return -1; 
}

void _sioInit(void){ // Function that handles the drivers and setup port
	sceHprmReset();
	/* Shut down the remote driver */
	sceHprmEnd();
	/* Enable UART 4 */
	sceSysregUartIoEnable(4);
	/* Enable remote control power */
	sceSysconCtrlHRPower(1);
}

void sioSetBaud(int baud){// Set & Calcule baud and clock ;) Set baud of sio
	// no need to export this....always call sioInit()...
	int div1, div2; // low, high bits of divisor value

	div1 = PSP_UART_CLK / baud;
	div2 = div1 & 0x3F;
	div1 >>= 6;

	_sw(div1, PSP_UART4_DIV1);
	_sw(div2, PSP_UART4_DIV2);
	_sw(0x60, PSP_UART4_CTRL); // ?? someone do it with 0x70
}

void sioInit(int baud){ // Main Function Starts all
	unsigned int k1 = pspSdkSetK1(0);
	_sioInit();
	sio_eventflag = sceKernelCreateEventFlag("SioShellEvent", 0, 0, 0); // Creamos la flag sio
	
	sceKernelRegisterIntrHandler(PSP_HPREMOTE_INT, 1, intr_handler, NULL, NULL); // Registramos el thread de recv
	sceKernelEnableIntr(PSP_HPREMOTE_INT);
	sceKernelDelayThread(2000000);
	sioSetBaud(baud); // Set Baud ;)
	pspSdkSetK1(k1); 
}

void sioFinalize(){ // Main function terminates all
	unsigned int k1 = pspSdkSetK1(0);
	sceSysconCtrlHRPower(0);
	sceKernelDeleteEventFlag(sio_eventflag);
	sceKernelReleaseIntrHandler(PSP_HPREMOTE_INT);
	sceKernelDisableIntr(PSP_HPREMOTE_INT);
	sceHprmReset();
	sceHprmInit();
	pspSdkSetK1(k1);
}

int sioAvailable(void){ // Availability function returns something greater than 0 if there is anything over buff
	return (unsigned int)(SERIAL_BUFFER_SIZE + rx_buffer.head - rx_buffer.tail) % SERIAL_BUFFER_SIZE;
}

int sioRead(void){ // Get char over buffer "ring - anillo" ;D
	//unsigned int k1 = pspSdkSetK1(0); pspSdkSetK1(k1);
	// if the head isn't ahead of the tail, we don't have any characters
	if (rx_buffer.head == rx_buffer.tail) {
		return -1;
	} else {
		unsigned char c = rx_buffer.buffer[rx_buffer.tail];
		rx_buffer.tail = (unsigned int)(rx_buffer.tail + 1) % SERIAL_BUFFER_SIZE;
		return c;
	}
}

void sioWrite(int ch){ // Write char to hw ;)
	// as you see this is _blocking_...not an issue for 
	// normal use as everithing doing I/O
	// should run in its own thread..in addition, HW FIFO isn't 
	// working at all by now, so queue should not be that long :)
	while(_lw(PSP_UART4_STAT) & PSP_UART_TXFULL); 
	_sw(ch, PSP_UART4_FIFO);
}

void sioWriteBuff(const char *data, int len){ // Write buffer to hw and set len
	unsigned int k1 = pspSdkSetK1(0);
	int i = 0;
	for(i = 0; i < len; i++){
		sioWrite(data[i]);
	}
	pspSdkSetK1(k1);
}

void sioPrint(const char *str){ // Write buffer to hw and calcule automatic len
	unsigned int k1 = pspSdkSetK1(0); 
	while (*str){
		sioWrite(*str);
		str++;
	}
	pspSdkSetK1(k1); 
}

// not used but required:
int module_start(SceSize args, void *argp){
	return 0;
}

int module_stop(){
	return 0;
}
