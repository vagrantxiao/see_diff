#include "stream.h"

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



/*
* useful conversions
*
*/
const char* pr_flow::stream::port_type_to_str( pr_flow::axi_port_t port )
{
	switch(port)
	{
		case HPC0:	return "HPC0";
		case HPC1:	return "HPC1";
		case HP0:	return "HP0";
		case HP1:	return "HP1";
		case HP2:	return "HP2";
		case HP3:	return "HP3";
		default: return "ERROR";
	}
}

pr_flow::axi_port_t pr_flow::stream::port_str_to_type( char* str )
{
	if( !str )
		return INVALID_PORT;

	for( int n=0; n < MAX_NUM_PORTS; n++ )
	{
		if( strcasecmp(str, port_type_to_str((axi_port_t)n)) == 0 )
			return (axi_port_t)n;
	}

	return INVALID_PORT;
}


const char* pr_flow::stream::mem_type_to_str( pr_flow::memory_t memory )
{
	switch(memory)
	{
		case DDR:	return "DDR";
		case OCM:	return "OCM";
		case CACHE:	return "CACHE";
		default: return "ERROR";
	}
}

pr_flow::memory_t pr_flow::stream::mem_str_to_type( char* str )
{
	if( !str )
		return INVALID_MEMORY;

	for( int n=0; n < MAX_NUM_MEMORIES; n++ )
	{
		if( strcasecmp(str, mem_type_to_str((pr_flow::memory_t)n)) == 0 )
			return (pr_flow::memory_t)n;
	}

	return INVALID_MEMORY;
}

// creates a stream with a buffer in OCM
pr_flow::stream::stream( pr_flow::stream_id_t stream_id, pr_flow::direction_t direction, pr_flow::axi_port_t port  )
{
	//
	this->buff = (volatile uint64_t*)get_memory(stream_id,direction);

	// set the our default buffer as non cacheable

//	Xil_SetTlbAttributes((UINTPTR)this->buff, NORM_NONCACHE);
//	dmb();


	Xil_DisableMPU();
//	Xil_SetMPURegion((INTPTR)this->buff,pr_flow::RAW_BUFFER_SIZE,MPU_REG_DISABLED);

//	Xil_SetTlbAttributes((UINTPTR)this->buff, NORM_NONCACHE);
//	dmb();

	//
	memset((void*)this->buff, 0, RAW_BUFFER_SIZE*sizeof(uint64_t));


	// set the unique characteristics of the stream
	this->ptr = 0;
	this->head = 0;
	this->tail = 0;
	this->port = port;
	this->memory = OCM;
	this->buff_size = MASK;
	this->direction = direction;
	this->bytes_written = 0;
	this->bytes_read = 0;
	this->bsp_id = 0;
	this->stream_id = stream_id;

}

//void stream::stream_destroy( stream_id_type stream_id)
pr_flow::stream::~stream()
{
	// empty desturctor as nothing is malloced in constructor
}

/*
* write using the circular buffer algorithm
* local copies and tail are refreshed on misses
* reducing the amount of reads from memory
*/
void pr_flow::stream::write( uint64_t data )
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
			this->tail = this->buff[TAIL_POINTER];
        }
        else
        {


        	//
        	// write and update local pointer
        	this->buff[this->head] = data;
		
			// ensures the data is written before the head pointer update is written to memory        	
        	dsb();

            this->head++;
            this->head = this->head & this->buff_size;


//            printf("head: %d tail: %d full: %d\n",this->head,this->tail,this->full);


            //
            // update state
            this->buff[HEAD_POINTER] = this->head;

            this->bytes_written++;
            break;
        }
	}
}

//
// reads one byte from the buffer
//
uint64_t pr_flow::stream::read()
{

	int timeout = 0;
	uint64_t data = 0;


	while(1)
	{

		if( (this->tail == this->head) )
		{
			//
			// not allowed to read when the pointers are equal and the full bit is zero
			// can read when pointers are equal and fullness is set this means writer is much faster
			//
			this->head = this->buff[HEAD_POINTER];
        }
		else
		{
			//
            // grab data and update pointers
			//
			data = this->buff[this->tail];
            this->tail++;
            this->tail = this->tail & this->buff_size;

//            printf("head: %d tail: %d full: %d\n",this->tail,this->tail,this->full);

            // write our new updated position
            this->buff[TAIL_POINTER] = this->tail;

            this->bytes_read++;
            return data;

		}
	}

	//
	return -1;
}
