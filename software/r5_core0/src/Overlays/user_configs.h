#ifndef _USER_CONFIGS_
#define _USER_CONFIGS_


#define HW_STREAMS 6
#define SW_STREAMS 10
#define BUFFER_SIZE_POW_2 512 // mustbe a power of 2 to work
#define ACTIVE_CORES 2

// which tcl design was used? this limits which render application you may run successfully
#define RENDER_COMPRESSED_TCL
//#define RENDER_TCL

#define NUM_RX_CONFIGS 1
#define NUM_TX_CONFIGS 1

#endif
