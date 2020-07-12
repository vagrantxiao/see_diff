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
#include "xstatus.h"

#include "mmu.h"
#include "benchmark_stream.h"


#include "stream_ip_bsp/bsp_jump_table.h"
#include "stream_ip_bsp/bert_example_jump/bert_example_jump.h"

// destructor frees memory associated in init
pr_flow::benchmark_stream::~benchmark_stream( )
{

}

// override default stream to a new IP and new memory
int pr_flow::benchmark_stream::stream_init( test_t test_id,memory_t mem )
{

	int rval = XST_FAILURE;

	//
	this->test = test_id;

    if( test != PS_MEMORY )
    {
    	// get our function pointers
    	this->bsp_table = get_jump_table(test_id,this->direction);
    }
	//
	this->memory = mem;

	//
	this->bsp_id = this->stream_id;

    u32 offset = 0;

   	// hi jack the already allocted buffer
	this->bench_buff = (uint64_t*)this->buff;

   	// get the address of our buffer
    offset = *((u32*)(&this->bench_buff));

    if( test != PS_MEMORY )
    {
    	// set the hls buffer address
    	rval = initialize_bip( offset,this->words );
    }
    else
    {
    	rval = XST_SUCCESS;
    }
 	// ensure drivers are created successfully
     if(rval != XST_SUCCESS)
     {
        xil_printf("error in init\r\n");
        return XST_FAILURE;
     }


   	// set as non cacheable
   	if( this->memory == DDR || this->memory == OCM )
   	{
   		// disable snooping
   		Xil_Out32(0xFD6E4000,0x0);
   		dmb();
   		Xil_SetTlbAttributes((UINTPTR)this->bench_buff, NORM_NONCACHE);
   		dmb();
   	}

   	// mark our memory regions as outer shareable which means it will not live in L1 but L2
   	if( this->memory == CACHE )
   	{
   		// enable snooping
   		Xil_Out32(0xFD6E4000,0x1);
   		dmb();
   		Xil_SetTlbAttributes((UINTPTR)this->bench_buff, 0x605);
   		dmb();
   	}
    return XST_SUCCESS;
}

int pr_flow::benchmark_stream::initialize_bip( uint32_t offset, uint8_t word )
{
	int rval = XST_FAILURE;
	// ensure bsp table is assigned
	assert( this->bsp_table->init != NULL );

	//
	ip_data_t ip;
	ip.stream_id = this->stream_id;
	ip.offset = offset;
	ip.words = word;
	ip.bsp_id =  this->stream_id;


	// initlaize our construct for talking to hardware
	rval = this->bsp_table->init(this->axi_config,(void*)&ip);
	return rval;
}

/*
* Simple read write with no handshaking
*/
void pr_flow::benchmark_stream::simple_write( uint64_t data )
{
	//
	this->bench_buff[this->ptr] = data;
	this->ptr++;
	this->ptr = this->ptr % this->buff_size;
}

/*
* Simple read write with no handshaking
*/
uint64_t pr_flow::benchmark_stream::simple_read()
{
	uint64_t temp;

	temp = this->bench_buff[this->ptr];
	this->ptr++;
	this->ptr = this->ptr % this->buff_size;

	//
	return temp;
}

/*
* issues 4 unrolled writes
*/
void pr_flow::benchmark_stream::unrolled_write( uint64_t data )
{
	//
	this->bench_buff[this->ptr] = data;
	this->bench_buff[this->ptr+1] = data+1;
	this->bench_buff[this->ptr+2] = data+2;
	this->bench_buff[this->ptr+3] = data+3;
	this->bench_buff[this->ptr+4] = data+4;
	this->bench_buff[this->ptr+5] = data+5;
	this->bench_buff[this->ptr+6] = data+6;
	this->bench_buff[this->ptr+7] = data+7;
	this->bench_buff[this->ptr+8] = data+8;
	this->bench_buff[this->ptr+9] = data+9;
	this->bench_buff[this->ptr+10] = data+10;
	this->bench_buff[this->ptr+11] = data+11;
	this->bench_buff[this->ptr+12] = data+12;
	this->bench_buff[this->ptr+13] = data+13;
	this->bench_buff[this->ptr+14] = data+14;
	this->bench_buff[this->ptr+15] = data+15;
	this->bench_buff[this->ptr+16] = data+16;
	this->bench_buff[this->ptr+17] = data+17;
	this->bench_buff[this->ptr+18] = data+18;
	this->bench_buff[this->ptr+19] = data+19;
	this->bench_buff[this->ptr+20] = data+20;
	this->bench_buff[this->ptr+21] = data+21;
	this->bench_buff[this->ptr+22] = data+22;
	this->bench_buff[this->ptr+23] = data+23;
	this->bench_buff[this->ptr+24] = data+24;
	this->bench_buff[this->ptr+25] = data+25;
	this->bench_buff[this->ptr+26] = data+26;
	this->bench_buff[this->ptr+27] = data+27;
	this->bench_buff[this->ptr+28] = data+28;
	this->bench_buff[this->ptr+29] = data+29;
	this->bench_buff[this->ptr+30] = data+30;
	this->bench_buff[this->ptr+31] = data+31;


	this->ptr+=32;
	this->ptr = this->ptr & this->buff_size;
}

/*
* issues 4 unrolled reads
*/
void pr_flow::benchmark_stream::unrolled_read( uint64_t* temp)
{
	temp[0] = this->bench_buff[this->ptr];
	temp[1] = this->bench_buff[this->ptr+1];
	temp[2] = this->bench_buff[this->ptr+2];
	temp[3] = this->bench_buff[this->ptr+3];
	temp[4] = this->bench_buff[this->ptr+4];
	temp[5] = this->bench_buff[this->ptr+5];
	temp[6] = this->bench_buff[this->ptr+6];
	temp[7] = this->bench_buff[this->ptr+7];
	temp[8] = this->bench_buff[this->ptr+8];
	temp[9] = this->bench_buff[this->ptr+9];
	temp[10] = this->bench_buff[this->ptr+10];
	temp[11] = this->bench_buff[this->ptr+11];
	temp[12] = this->bench_buff[this->ptr+12];
	temp[13] = this->bench_buff[this->ptr+13];
	temp[14] = this->bench_buff[this->ptr+14];
	temp[15] = this->bench_buff[this->ptr+15];
	temp[16] = this->bench_buff[this->ptr+16];
	temp[17] = this->bench_buff[this->ptr+17];
	temp[18] = this->bench_buff[this->ptr+18];
	temp[19] = this->bench_buff[this->ptr+19];
	temp[20] = this->bench_buff[this->ptr+20];
	temp[21] = this->bench_buff[this->ptr+21];
	temp[22] = this->bench_buff[this->ptr+22];
	temp[23] = this->bench_buff[this->ptr+23];
	temp[24] = this->bench_buff[this->ptr+24];
	temp[25] = this->bench_buff[this->ptr+25];
	temp[26] = this->bench_buff[this->ptr+26];
	temp[27] = this->bench_buff[this->ptr+27];
	temp[28] = this->bench_buff[this->ptr+28];
	temp[29] = this->bench_buff[this->ptr+29];
	temp[30] = this->bench_buff[this->ptr+30];
	temp[31] = this->bench_buff[this->ptr+31];

	this->ptr+=32;
	this->ptr = this->ptr & this->buff_size;

}

void pr_flow::benchmark_stream::presence_write( uint32_t data )
{

	while(1)
	{
		volatile uint64_t temp = this->bench_buff[this->ptr];

		// bit is high meaning that the data is not consumed yet
		if( (temp & PRESENCE_BIT) == PRESENCE_BIT )
		{
			// wait some time
		}
		else
		{
			// set the bit and write the data
			this->bench_buff[this->ptr] = data | PRESENCE_BIT;
			this->ptr++;
			this->ptr = this->ptr % this->buff_size;
			break;
		}
	}
}

uint32_t pr_flow::benchmark_stream::presence_read()
{

	while(1)
	{
		// read data
		volatile uint64_t temp = this->bench_buff[this->ptr];

		// bit is high meaning that the data is ready to be consumed
		if( (temp & PRESENCE_BIT) == PRESENCE_BIT )
		{
			// clear the bit
			this->bench_buff[this->ptr] = 0;
			this->ptr++;
			this->ptr = this->ptr % this->buff_size;
			return (temp & MASK32);
		}
		else
		{
			// miss and wait
		}
	}

	//
	return -1;
}

/*
* write using the circular buffer algorithm
* local copies and tail are refreshed on misses
* reducing the amount of reads from memory
*/
void pr_flow::benchmark_stream::primitive_write( uint64_t data )
{

	while(1)
	{

		//
		// Not allowed to write when stream is full or the next update will collide
		// always leave 1 space
		if( ( (this->head + 1)  == this->tail) || (( (this->head +1) & this->buff_size) ==  this->tail ) )
		{
			//
			//
			this->tail = this->bench_buff[TAIL_POINTER];
        }
        else
        {


        	//
        	// write and update local pointer
        	this->bench_buff[this->head] = data;

			// ensures the data is written before the head pointer update is written to memory
        	dsb();

            this->head++;
            this->head = this->head & this->buff_size;


#ifdef DEBUG_B_STREAMS
            printf("head: %d tail: %d \n",this->head,this->tail);
#endif


            //
            // update state
            this->bench_buff[HEAD_POINTER] = this->head;

#ifdef DEBUG_B_STREAMS_COUNT
            this->bytes_written++;
#endif
            break;
        }
	}
}

//
// reads one byte from the buffer
//
uint64_t pr_flow::benchmark_stream::primitive_read()
{
	uint64_t data;

	while(1)
	{

		if( (this->tail == this->head) )
		{
			//
			// not allowed to read when the pointers are equal and the full bit is zero
			// can read when pointers are equal and fullness is set this means writer is much faster
			//
			this->head = this->bench_buff[HEAD_POINTER];
        }
		else
		{
			//
            // grab data and update pointers
			//
			data = this->bench_buff[this->tail];
            this->tail++;
            this->tail = this->tail & this->buff_size;

#ifdef DEBUG_B_STREAMS
            printf("head: %d tail: %d \n",this->head,this->tail);
#endif

            // write our new updated position
            this->bench_buff[TAIL_POINTER] = this->tail;

#ifdef DEBUG_B_STREAMS_COUNT
            this->bytes_read++;
#endif
            return data;

		}
	}
	return -1;
}


const char* pr_flow::benchmark_stream::test_type_to_str( pr_flow::test_t test )
{
	switch(test)
	{
		case LOOP_BACK32:	return "LOOP_BACK32";
		case LOOP_BACK64:	return "LOOP_BACK64";
		case LOOP_BACK128:	return "LOOP_BACK128";
		case LOOP_BACK128_LITE:	return "LOOP_BACK128_LITE";
		case READ_ONLY32:	return "READ_ONLY32";
		case READ_ONLY64:	return "READ_ONLY64";
		case READ_ONLY128:	return "READ_ONLY128";
		case READ_ONLY128_BURST:	return "READ_ONLY128_BURST";
		case WRITE_ONLY32:	return "WRITE_ONLY32";
		case WRITE_ONLY64:	return "WRITE_ONLY64";
		case WRITE_ONLY128:	return "WRITE_ONLY128";
		case WRITE_ONLY128_BURST:	return "WRITE_ONLY128_BURST";
		case PRESENCE_2CORE: return "PRESENCE_2CORE";
		case CIRCULAR_2CORE: return "CIRCULAR_2CORE";
		case CIRCULAR_2CORE_128: return "CIRCULAR_2CORE_128";
		default: return "ERROR";
	}
}

pr_flow::test_t pr_flow::benchmark_stream::test_str_to_type( char* str )
{
	if( !str )
		return INVALID_TEST;
	// iterate over possible tests
	for( int n=0; n < MAX_TEST; n++ )
	{
		if( strcasecmp(str, test_type_to_str((test_t)n)) == 0 )
			return (test_t)n;
	}

	return INVALID_TEST;
}

int16_t pr_flow::benchmark_stream::get_width( pr_flow::test_t test )
{
	// returns how many bytes of data are moved for each test
	switch(test)
	{
		case LOOP_BACK32:	return sizeof(uint32_t);
		case LOOP_BACK64:	return sizeof(uint64_t);
		case LOOP_BACK128:	return sizeof(uint64_t)*2;
		case LOOP_BACK128_LITE:	return sizeof(uint64_t)*2;
		case READ_ONLY32:	return sizeof(uint32_t);
		case READ_ONLY64:	return sizeof(uint64_t);
		case READ_ONLY128:	return sizeof(uint64_t)*2;
		case READ_ONLY128_BURST:	return sizeof(uint64_t)*2;
		case WRITE_ONLY32:	return sizeof(uint32_t);
		case WRITE_ONLY64:	return sizeof(uint64_t);
		case WRITE_ONLY128:	return sizeof(uint64_t)*2;
		case WRITE_ONLY128_BURST:	return sizeof(uint64_t)*2;
		case PRESENCE_2CORE: return sizeof(uint32_t);
		case CIRCULAR_2CORE: return sizeof(uint64_t);
		case CIRCULAR_2CORE_128: return sizeof(uint64_t)*2;
		default: return 0;
	}
}

void pr_flow::benchmark_stream::get_bert_ptrs()
{
	bert_buffs_t temp;
	// ensure bsp table is assigned
//	assert( this->ext_table->get_ptrs != NULL );
//	// call the stop function from function pointer table
//	this->ext_table->get_ptrs(this->axi_config,&temp);

	this->bert_in = (uint32_t*)temp.in;
	this->bert_out = (uint32_t*)temp.out;
}

void pr_flow::benchmark_stream::set_bert_vars( uint32_t a )
{
	// ensure bsp table is assigned
//	assert( this->ext_table->set_vars != NULL );
//	// call the is done from function pointer table
//	this->ext_table->set_vars(this->axi_config,(void*)&a);
}

uint32_t pr_flow::benchmark_stream::read_bert_in()
{
	uint32_t temp=0;
	temp = this->bert_in[this->ptr];
	this->ptr++;
	this->ptr = this->ptr % 10000;
	return temp;
}

void pr_flow::benchmark_stream::write_bert_in( uint32_t a )
{
	this->bert_in[this->ptr] = a;
	this->ptr++;
	this->ptr = this->ptr % 10000;
}

uint32_t pr_flow::benchmark_stream::read_bert_out()
{
	uint32_t temp=0;
	temp = this->bert_out[this->ptr];
	this->ptr++;
	this->ptr = this->ptr % 10000;
	return temp;
}

void pr_flow::benchmark_stream::write_bert_out( uint32_t a )
{
	this->bert_out[this->ptr] = a;
	this->ptr++;
	this->ptr = this->ptr % 10000;
}

