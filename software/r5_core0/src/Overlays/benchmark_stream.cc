#include <stdint.h>

#include <stdlib.h>
#include <strings.h>
#include <assert.h>
#include <stdio.h>
#include "sleep.h"

#include "xil_cache.h"
#include "xil_mmu.h"
#include "xpseudo_asm.h"
#include "xil_types.h"
#include "xparameters.h"
#include "mmu.h"
#include "benchmark_stream.h"

//includes for test_ip
#include "xloop_back_32_read.h"
#include "xloop_back_64_read.h"
#include "xloop_back_128_read.h"
#include "xloop_back_32_write.h"
#include "xloop_back_64_write.h"
#include "xloop_back_128_write.h"
#include "xread_only_32.h"
#include "xread_only_64.h"
#include "xread_only_128.h"
#include "xwrite_only_32.h"
#include "xwrite_only_64.h"
#include "xwrite_only_128.h"
#include "xcirc_buff_read.h"
#include "xcirc_buff_write.h"
#include "xpresence_write.h"
#include "xpresence_read.h"

static int16_t ip128[XPAR_XLOOP_BACK_128_READ_NUM_INSTANCES] = {XPAR_LOOP_BACK_128_READ_0_DEVICE_ID,XPAR_LOOP_BACK_128_READ_1_DEVICE_ID};
static int16_t ip64[XPAR_XLOOP_BACK_64_READ_NUM_INSTANCES] = {XPAR_LOOP_BACK_64_READ_0_DEVICE_ID,XPAR_LOOP_BACK_64_READ_1_DEVICE_ID};
static int16_t ip32[XPAR_XLOOP_BACK_32_READ_NUM_INSTANCES] = {XPAR_LOOP_BACK_32_READ_0_DEVICE_ID,XPAR_LOOP_BACK_32_READ_1_DEVICE_ID};

static int16_t read_only_ip32[XPAR_XREAD_ONLY_64_NUM_INSTANCES] = {XPAR_READ_ONLY_32_0_DEVICE_ID,XPAR_READ_ONLY_32_1_DEVICE_ID};
static int16_t write_only_ip32[XPAR_XWRITE_ONLY_64_NUM_INSTANCES] = {XPAR_WRITE_ONLY_32_0_DEVICE_ID,XPAR_WRITE_ONLY_32_1_DEVICE_ID};

static int16_t read_only_ip64[XPAR_XREAD_ONLY_64_NUM_INSTANCES] = {XPAR_READ_ONLY_64_0_DEVICE_ID,XPAR_READ_ONLY_64_1_DEVICE_ID};
static int16_t write_only_ip64[XPAR_XWRITE_ONLY_64_NUM_INSTANCES] = {XPAR_WRITE_ONLY_64_0_DEVICE_ID,XPAR_WRITE_ONLY_64_1_DEVICE_ID};

static int16_t read_only_ip128[XPAR_XREAD_ONLY_128_NUM_INSTANCES] = {XPAR_READ_ONLY_128_0_DEVICE_ID,XPAR_READ_ONLY_128_1_DEVICE_ID};
static int16_t write_only_ip128[XPAR_XWRITE_ONLY_128_NUM_INSTANCES] = {XPAR_WRITE_ONLY_128_0_DEVICE_ID,XPAR_WRITE_ONLY_128_1_DEVICE_ID};

static int16_t circular_read_ip[XPAR_XCIRC_BUFF_READ_NUM_INSTANCES] = {XPAR_CIRC_BUFF_READ_0_DEVICE_ID};

//static int16_t circular_write_ip[XPAR_XCIRC_BUFF_WRITE_NUM_INSTANCES] = {XPAR_CIRC_BUFF_WRITE_0_DEVICE_ID};

static int16_t presence_read_ip[XPAR_XPRESENCE_READ_NUM_INSTANCES] = {XPAR_PRESENCE_READ_0_DEVICE_ID};
//static int16_t presence_read_ip[1] = {0};
//static int16_t presence_write_ip[XPAR_XWRITE_ONLY_128_NUM_INSTANCES] = {XPAR_WRITE_ONLY_128_0_DEVICE_ID};

//static pr_flow::test_t test_table[pr_flow::MAX_TEST] = {pr_flow::LOOP_BACK32,pr_flow::LOOP_BACK64,pr_flow::LOOP_BACK128,pr_flow::READ_ONLY32,pr_flow::READ_ONLY64,pr_flow::READ_ONLY128,pr_flow::WRITE_ONLY32,pr_flow::WRITE_ONLY64,pr_flow::WRITE_ONLY128};
// this needs to correspond with the indices above so we can do a loop up
static int16_t* ip_map[pr_flow::MAX_TEST] = {ip32,ip64,ip128,read_only_ip32,read_only_ip64,read_only_ip128,write_only_ip32,write_only_ip64,write_only_ip128,presence_read_ip,circular_read_ip};

// destructor frees memory associated in init
pr_flow::benchmark_stream::~benchmark_stream( )
{
	if( direction == TX )
	{
		free(this->axi_config_tx);
		this->axi_config_tx = NULL;
	}
	else if( direction == RX )
	{
		free(this->axi_config_rx);
		this->axi_config_rx = NULL;
	}
	// OCM is not heap managed
	if(this->memory != OCM)
		free((void*)this->buff);

	this->buff = NULL;
}

//
// pass in which port you want and who is master and slave for this stream
int pr_flow::benchmark_stream::stream_init( test_t test_id,memory_t mem )
{

	int rval = XST_FAILURE;

	//
	this->test = test_id;

	//
	this->memory = mem;

	//
	this->bsp_id = ip_map[test_id][this->stream_id];

	// OCM is default the benchmarks can over ride this here
	if( mem == CACHE || mem == DDR )
	{
		this->buff = (volatile uint64_t*)malloc( sizeof(uint64_t)*this->buff_size );
		assert(this->buff != NULL);
	}

	// depending on the direction get the correct IP instance
	if( direction == TX )
	{
		this->axi_config_tx = (bsp_template_t*)malloc( sizeof(bsp_template_t) );
		this->axi_config_rx = NULL;
		assert(this->axi_config_tx != NULL);
		rval = initialize_ip();
	}
	else if( direction == RX )
	{
		this->axi_config_tx = NULL;
		this->axi_config_rx = (bsp_template_t*)malloc( sizeof(bsp_template_t) );
		assert(this->axi_config_rx != NULL);
		rval = initialize_ip();
	}
	else if( direction == SW_SHARED )
	{
		// any routines can go here
		rval = XST_SUCCESS;
	}
	else
	{
		// invalid arg crash the program
		assert(0);
	}

	// ensure drivers are created successfully
    if(rval != XST_SUCCESS)
    {
       	xil_printf("error in init\r\n");
       	return XST_FAILURE;
    }


     // comppilers are stupid and this is the only way I can get rid of the error without losing precision when passed to set input
     u32 offset = *((u32*)(&buff));


     // initialize our drivers
     set_ptrs( offset );


	// slave 3 from CCI man page. This enables snooping from the HPC0 and 1 ports
	if( this->memory == DDR || this->memory == OCM )
	{
    	Xil_Out32(0xFD6E4000,0x0);
    	dmb();
		Xil_SetTlbAttributes((UINTPTR)this->buff, NORM_NONCACHE);
		dmb();
    }

    // mark our memory regions as outer shareable which means it will not live in L1 but L2
	if( this->memory == CACHE )
	{
    	Xil_Out32(0xFD6E4000,0x1);
    	dmb();
		Xil_SetTlbAttributes((UINTPTR)this->buff, 0x605);
		dmb();
	}
    return XST_SUCCESS;
}


/*
* Simple read write with no handshaking
*/
void pr_flow::benchmark_stream::simple_write( uint32_t data )
{
	//
	this->buff[this->ptr] = data;
	this->ptr++;
	this->ptr = this->ptr % this->buff_size;
}

/*
* Simple read write with no handshaking
*/
uint32_t pr_flow::benchmark_stream::simple_read()
{
	uint32_t temp;

	temp = this->buff[this->ptr];
	this->ptr++;
	this->ptr = this->ptr % this->buff_size;

	//
	return temp;
}


uint32_t pr_flow::benchmark_stream::is_stream_done()
{
	if(this->direction == TX )
	{
		if( this->test == LOOP_BACK32 )
		{
			return XLoop_back_32_read_IsDone((XLoop_back_32_read*)this->axi_config_tx);
		}
		else if( this->test == LOOP_BACK64 )
		{
			return XLoop_back_64_read_IsDone((XLoop_back_64_read*)this->axi_config_tx);
		}
		else if( this->test == LOOP_BACK128 )
		{
			return XLoop_back_128_read_IsDone((XLoop_back_128_read*)this->axi_config_tx);
		}
		else if( this->test == READ_ONLY32 )
		{
			return XRead_only_32_IsDone( (XRead_only_32*)this->axi_config_tx );
		}
		else if( this->test == READ_ONLY64 )
		{
			return XRead_only_64_IsDone( (XRead_only_64*)this->axi_config_tx );
		}
		else if( this->test == READ_ONLY128 )
		{
			return XRead_only_128_IsDone( (XRead_only_128*)this->axi_config_tx );
		}
	}
	else if(this->direction == RX)
	{
		if( this->test == LOOP_BACK32 )
		{
			return XLoop_back_32_write_IsDone((XLoop_back_32_write*)this->axi_config_rx);
		}
		else if( this->test == LOOP_BACK64 )
		{
			return XLoop_back_64_write_IsDone((XLoop_back_64_write*)this->axi_config_rx);
		}
		else if( this->test == LOOP_BACK128 )
		{
			return XLoop_back_128_write_IsDone((XLoop_back_128_write*)this->axi_config_rx);
		}
		else if( this->test == WRITE_ONLY32 )
		{
			return XWrite_only_32_IsDone( (XWrite_only_32*)this->axi_config_rx );
		}
		else if( this->test == WRITE_ONLY64 )
		{
			return XWrite_only_64_IsDone( (XWrite_only_64*)this->axi_config_rx );
		}
		else if( this->test == WRITE_ONLY128 )
		{
			return XWrite_only_128_IsDone( (XWrite_only_128*)this->axi_config_rx );
		}
	}
	return 0;
}

void pr_flow::benchmark_stream::start_stream()
{
	if(this->direction == TX )
	{
		if( this->test == LOOP_BACK32 )
		{
			XLoop_back_32_read_Start((XLoop_back_32_read*)this->axi_config_tx);
		}
		else if( this->test == LOOP_BACK64 )
		{
			XLoop_back_64_read_Start((XLoop_back_64_read*)this->axi_config_tx);
		}
		else if( this->test == LOOP_BACK128 )
		{
			XLoop_back_128_read_Start((XLoop_back_128_read*)this->axi_config_tx);
		}
		else if( this->test == READ_ONLY32 )
		{
			XRead_only_32_Start( (XRead_only_32*)this->axi_config_tx );
		}
		else if( this->test == READ_ONLY64 )
		{
			XRead_only_64_Start( (XRead_only_64*)this->axi_config_tx );
		}
		else if( this->test == READ_ONLY128 )
		{
			XRead_only_128_Start( (XRead_only_128*)this->axi_config_tx );
		}
		else if( this->test == PRESENCE_2CORE )
		{
			XPresence_read_EnableAutoRestart( (XPresence_read*)this->axi_config_tx );
			XPresence_read_Start( (XPresence_read*)this->axi_config_tx );
		}
		else if( this->test == CIRCULAR_2CORE )
		{
			XCirc_buff_read_EnableAutoRestart( (XCirc_buff_read*)this->axi_config_tx );
			XCirc_buff_read_Start( (XCirc_buff_read*)this->axi_config_tx );
		}
	}
	else if(this->direction == RX)
	{
		if( this->test == LOOP_BACK32 )
		{
			XLoop_back_32_write_Start((XLoop_back_32_write*)this->axi_config_rx);
		}
		else if( this->test == LOOP_BACK64 )
		{
			XLoop_back_64_write_Start((XLoop_back_64_write*)this->axi_config_rx);
		}
		else if( this->test == LOOP_BACK128 )
		{
			XLoop_back_128_write_Start((XLoop_back_128_write*)this->axi_config_rx);
		}
		else if( this->test == WRITE_ONLY32 )
		{
			XWrite_only_32_Start( (XWrite_only_32*)this->axi_config_rx );
		}
		else if( this->test == WRITE_ONLY64 )
		{
			XWrite_only_64_Start( (XWrite_only_64*)this->axi_config_rx );
		}
		else if( this->test == WRITE_ONLY128 )
		{
			XWrite_only_128_Start( (XWrite_only_128*)this->axi_config_rx );
		}
		else if( this->test == PRESENCE_2CORE )
		{
			XPresence_write_EnableAutoRestart( (XPresence_write*)this->axi_config_rx );
			XPresence_write_Start( (XPresence_write*)this->axi_config_rx );
		}
		else if( this->test == CIRCULAR_2CORE )
		{
			XCirc_buff_write_EnableAutoRestart( (XCirc_buff_write*)this->axi_config_rx );
			XCirc_buff_write_Start( (XCirc_buff_write*)this->axi_config_rx );
		}
	}

}

/*
* presence bit algorithm
* check to see if the 33rd bit if it is low it is safe to write
* if it is high we must wait
*/
void pr_flow::benchmark_stream::presence_write( uint32_t data )
{

	while(1)
	{
		volatile uint64_t temp = this->buff[this->ptr];

		// bit is high meaning that the data is not consumed yet
		if( (temp & PRESENCE_BIT) == PRESENCE_BIT )
		{
			// wait some time
		}
		else
		{
			// set the bit and write the data
			this->buff[this->ptr] = data | PRESENCE_BIT;
			this->ptr++;
			this->ptr = this->ptr % this->buff_size;
			break;
		}
	}
}

/*
* presence bit algorithm
* check to see if the 33rd bit if it is high if it is it is safe to read
* if it is low we must wait
*/
uint32_t pr_flow::benchmark_stream::presence_read()
{

	while(1)
	{
		// read data
		volatile uint64_t temp = this->buff[this->ptr];

		// bit is high meaning that the data is ready to be consumed
		if( (temp & PRESENCE_BIT) == PRESENCE_BIT )
		{
			// clear the bit
			//			stream->buff[stream->ptr] &= (~PRESENCE_BIT);
			this->buff[this->ptr] = 0;
			this->ptr++;
			this->ptr = this->ptr % this->buff_size;
			return (temp & MASK32);
		}
		else
		{
			// miss
		}
	}

	//
	return -1;
}

void pr_flow::benchmark_stream::print_state( )
{
	//printf("head : %d tail : %d read : %d wrote : %d \n",this->head,this->tail,this->bytes_read,this->bytes_written);
	if(this->direction == TX )
	{
		//printf("IsDone: %d \n",XCirc_buff_read_many_IsDone(this->axi_config_tx));
		printf("Input %X \n",XPresence_read_Get_input_r( (XPresence_read*)this->axi_config_tx ));
	}
	else if(this->direction == RX)
	{
//		printf("IsDone: %d \n",XCirc_buff_write_many_IsDone(this->axi_config_rx));
		printf("Input %X \n",XPresence_write_Get_output_r( (XPresence_write*)this->axi_config_rx ));
	}
}

const char* pr_flow::benchmark_stream::test_type_to_str( pr_flow::test_t test )
{
	switch(test)
	{
		case LOOP_BACK32:	return "LOOP_BACK32";
		case LOOP_BACK64:	return "LOOP_BACK64";
		case LOOP_BACK128:	return "LOOP_BACK128";
		case READ_ONLY32:	return "READ_ONLY32";
		case READ_ONLY64:	return "READ_ONLY64";
		case READ_ONLY128:	return "READ_ONLY128";
		case WRITE_ONLY32:	return "WRITE_ONLY32";
		case WRITE_ONLY64:	return "WRITE_ONLY64";
		case WRITE_ONLY128:	return "WRITE_ONLY128";
		case PRESENCE_2CORE: return "PRESENCE_2CORE";
		case CIRCULAR_2CORE: return "CIRCULAR_2CORE";
		default: return "ERROR";
	}
}

pr_flow::test_t pr_flow::benchmark_stream::test_str_to_type( char* str )
{
	if( !str )
		return INVALID_TEST;

	for( int n=0; n < MAX_TEST; n++ )
	{
		if( strcasecmp(str, test_type_to_str((test_t)n)) == 0 )
			return (test_t)n;
	}

	return INVALID_TEST;
}

int16_t pr_flow::benchmark_stream::get_width()
{
	switch(this->test)
	{
		case LOOP_BACK32:	return sizeof(uint32_t);
		case LOOP_BACK64:	return sizeof(uint64_t);
		case LOOP_BACK128:	return sizeof(uint64_t)*2;
		case READ_ONLY32:	return sizeof(uint32_t);
		case READ_ONLY64:	return sizeof(uint64_t);
		case READ_ONLY128:	return sizeof(uint64_t)*2;
		case WRITE_ONLY32:	return sizeof(uint32_t);
		case WRITE_ONLY64:	return sizeof(uint64_t);
		case WRITE_ONLY128:	return sizeof(uint64_t)*2;
		case PRESENCE_2CORE: return sizeof(uint32_t);
		case CIRCULAR_2CORE: return sizeof(uint64_t);
		default: return 0;
	}
}

// TODO chang to a function of ptrs...
int pr_flow::benchmark_stream::initialize_ip()
{
	int rval = XST_FAILURE;
	// initialize our drivers
	if(this->direction == TX )
	{
		if( this->test == LOOP_BACK32 )
		{
			rval = XLoop_back_32_read_Initialize( (XLoop_back_32_read*)this->axi_config_tx, this->bsp_id );
		}
		else if( this->test == LOOP_BACK64 )
		{
			rval = XLoop_back_64_read_Initialize( (XLoop_back_64_read*)this->axi_config_tx, this->bsp_id );
		}
		else if( this->test == LOOP_BACK128 )
		{
			rval = XLoop_back_128_read_Initialize( (XLoop_back_128_read*)this->axi_config_tx, this->bsp_id );
		}
		else if( this->test == READ_ONLY32 )
		{
			rval = XRead_only_32_Initialize( (XRead_only_32*)this->axi_config_tx, this->bsp_id );
		}
		else if( this->test == READ_ONLY64 )
		{
			rval = XRead_only_64_Initialize( (XRead_only_64*)this->axi_config_tx, this->bsp_id );
		}
		else if( this->test == READ_ONLY128 )
		{
			rval = XRead_only_128_Initialize( (XRead_only_128*)this->axi_config_tx, this->bsp_id );
		}
		else if( this->test == PRESENCE_2CORE )
		{
			rval = XPresence_read_Initialize( (XPresence_read*)this->axi_config_tx, this->bsp_id );
		}
		else if( this->test == CIRCULAR_2CORE )
		{
			rval = XCirc_buff_read_Initialize( (XCirc_buff_read*)this->axi_config_tx, this->bsp_id );
		}
	}
	else if(this->direction == RX)
	{
		if( this->test == LOOP_BACK32 )
		{
			rval = XLoop_back_32_write_Initialize( (XLoop_back_32_write*)this->axi_config_rx, this->bsp_id );
		}
		else if( this->test == LOOP_BACK64 )
		{
			rval = XLoop_back_64_write_Initialize( (XLoop_back_64_write*)this->axi_config_rx, this->bsp_id );
		}
		else if( this->test == LOOP_BACK128 )
		{
			rval = XLoop_back_128_write_Initialize( (XLoop_back_128_write*)this->axi_config_rx, this->bsp_id );
		}
		else if( this->test == WRITE_ONLY32 )
		{
			rval = XWrite_only_32_Initialize( (XWrite_only_32*)this->axi_config_rx, this->bsp_id );
		}
		else if( this->test == WRITE_ONLY64 )
		{
			rval = XWrite_only_64_Initialize( (XWrite_only_64*)this->axi_config_rx, this->bsp_id );
		}
		else if( this->test == WRITE_ONLY128 )
		{
			rval = XWrite_only_128_Initialize( (XWrite_only_128*)this->axi_config_rx, this->bsp_id );
		}
		else if( this->test == PRESENCE_2CORE )
		{
			rval = XPresence_write_Initialize( (XPresence_write*)this->axi_config_rx, this->bsp_id );
		}
		else if( this->test == CIRCULAR_2CORE )
		{
			rval = XCirc_buff_write_Initialize( (XCirc_buff_write*)this->axi_config_rx, this->bsp_id );
		}
	}
	return rval;
}

void pr_flow::benchmark_stream::set_ptrs( u32 offset )
{

	// initialize our drivers
	if(this->direction == TX )
	{
		if( this->test == LOOP_BACK32 )
		{
			XLoop_back_32_read_Set_input_r( (XLoop_back_32_read*)this->axi_config_tx, offset );
		}
		else if( this->test == LOOP_BACK64 )
		{
			XLoop_back_64_read_Set_input_r( (XLoop_back_64_read*)this->axi_config_tx, offset );
		}
		else if( this->test == LOOP_BACK128 )
		{
			XLoop_back_128_read_Set_input_V( (XLoop_back_128_read*)this->axi_config_tx, offset );
		}
		else if( this->test == READ_ONLY32 )
		{
			XRead_only_32_Set_input_r( (XRead_only_32*)this->axi_config_tx, offset );
		}
		else if( this->test == READ_ONLY64 )
		{
			XRead_only_64_Set_input_r( (XRead_only_64*)this->axi_config_tx, offset );
		}
		else if( this->test == READ_ONLY128 )
		{
			XRead_only_128_Set_input_V( (XRead_only_128*)this->axi_config_tx, offset );
		}
		else if( this->test == PRESENCE_2CORE )
		{
			XPresence_read_Set_input_r( (XPresence_read*)this->axi_config_tx, offset );
		}
		else if( this->test == CIRCULAR_2CORE )
		{
			XCirc_buff_read_Set_input_r( (XCirc_buff_read*)this->axi_config_tx, offset );
		}
	}
	else if(this->direction == RX)
	{
		if( this->test == LOOP_BACK32 )
		{
			XLoop_back_32_write_Set_output_r( (XLoop_back_32_write*)this->axi_config_rx, offset );
		}
		else if( this->test == LOOP_BACK64 )
		{
			XLoop_back_64_write_Set_output_r( (XLoop_back_64_write*)this->axi_config_rx, offset );
		}
		else if( this->test == LOOP_BACK128 )
		{
			XLoop_back_128_write_Set_output_V( (XLoop_back_128_write*)this->axi_config_rx, offset );
		}
		else if( this->test == WRITE_ONLY32 )
		{
			XWrite_only_32_Set_output_r( (XWrite_only_32*)this->axi_config_rx, offset );
		}
		else if( this->test == WRITE_ONLY64 )
		{
			XWrite_only_64_Set_output_r( (XWrite_only_64*)this->axi_config_rx, offset );
		}
		else if( this->test == WRITE_ONLY128 )
		{
			XWrite_only_128_Set_output_V( (XWrite_only_128*)this->axi_config_rx, offset );
		}
		else if( this->test == PRESENCE_2CORE )
		{
			XPresence_write_Set_output_r( (XPresence_write*)this->axi_config_rx, offset );
		}
		else if( this->test == CIRCULAR_2CORE )
		{
			XCirc_buff_write_Set_output_r( (XCirc_buff_write*)this->axi_config_rx, offset );
		}

	}
}

void pr_flow::benchmark_stream::stop_ip()
{

	// initialize our drivers
	if(this->direction == TX )
	{
		if( this->test == PRESENCE_2CORE )
		{
			XPresence_read_DisableAutoRestart( (XPresence_read*)this->axi_config_tx );

			//
			while(XPresence_read_IsDone( (XPresence_read*)this->axi_config_tx ))
			{

			}
		}
		else if( this->test == CIRCULAR_2CORE )
		{
			//
			XCirc_buff_read_DisableAutoRestart( (XCirc_buff_read*)this->axi_config_tx );

			//
			while(XCirc_buff_read_IsDone( (XCirc_buff_read*)this->axi_config_tx ))
			{

			}
		}
	}
	else if(this->direction == RX)
	{
		if( this->test == PRESENCE_2CORE )
		{
			XPresence_write_DisableAutoRestart( (XPresence_write*)this->axi_config_rx );

			//
			while(XPresence_write_IsDone( (XPresence_write*)this->axi_config_rx ))
			{

			}
		}
		else if( this->test == CIRCULAR_2CORE )
		{
			//
			XCirc_buff_write_DisableAutoRestart( (XCirc_buff_write*)this->axi_config_rx );

			//
			while(XCirc_buff_write_IsDone( (XCirc_buff_write*)this->axi_config_rx ))
			{

			}
		}

	}
}


