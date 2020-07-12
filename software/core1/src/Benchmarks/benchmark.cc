// libs
#include <stdio.h>

#include <stdlib.h>
#include "xil_cache.h"
#include "xil_mmu.h"
#include "xpseudo_asm.h"
#include "xil_types.h"
#include "xparameters.h"
#include "xil_io.h"
#include "xil_exception.h"
#include "xscugic.h"
#include "sleep.h"
#include "xtime_l.h"

// my stuff
#include "../Overlays/stream.h"
#include "../Overlays/benchmark_stream.h"
#include "../Overlays/mmu.h"
#include "assert.h"
#include "xmutex.h"

//#define RUNNING_R5 // in test setup r5 and core1 talk

#define TIMER (0xFFFEB0016) // shared memory region for creation of sw streams


// only a 2 core example
int run_benchmark_ps_ps_4core( pr_flow::test_t test, pr_flow::memory_t memory, pr_flow::axi_port_t port )
{
	int loops = 100;
	uint64_t data =0;
	pr_flow::wide_t data1;
	XTime timer_start;

	data1.lower_32 = 0;
	data1.upper_32 = 0;

	volatile XTime* ptr = (volatile XTime*)TIMER;
	Xil_SetTlbAttributes((UINTPTR)ptr, NORM_NONCACHE);
	dmb();

	//
	//
	// Create rx stream
	pr_flow::benchmark_stream Stream( pr_flow::stream_id_t::STREAM_ID_0, pr_flow::direction_t::SW_SHARED,pr_flow::width_t::SW_WIDTH, port,memory );

	synchronize();

	XTime_GetTime(&timer_start);
	*ptr = timer_start;

	//
	//
	// Send data
	for(int w = 0; w < loops; w++)
	for(int k = 0; k < loops; k++)
	for(int j = 0; j < loops; j++)
	for(int i = 0; i < loops; i++)
	{
	    if(test == pr_flow::CIRCULAR_2CORE_128){
	    	Stream.write(data1);
	    }
	    else if(test == pr_flow::CIRCULAR_2CORE){
	    	Stream.primitive_write(data);
	    }
	    data1.lower_32++;
	    data1.upper_32++;
	    data++;

//			works
//			if(rand() % 15 == 11)
//			{
//				usleep( (rand()%20) );
//			}


	}

	//
	//
	// destroy streams
	Stream.~stream();

	synchronize();

	return XST_SUCCESS;
}

int run_benchmark_ps_pl_4core( pr_flow::test_t test, pr_flow::memory_t memory, pr_flow::axi_port_t port, pr_flow::width_t width )
{
	int loops = 100;
	uint64_t data =0;
	pr_flow::wide_t data1;
	XTime timer_start;
	volatile XTime* ptr = (volatile XTime*)TIMER;
	XTime_StartTimer();

	Xil_SetTlbAttributes((UINTPTR)ptr, NORM_NONCACHE);
	dmb();

	//
	//
	// Create rx stream
	pr_flow::benchmark_stream Stream1( pr_flow::stream_id_t::STREAM_ID_0, pr_flow::direction_t::TX,width, port,memory );

	synchronize();

	Stream1.start_stream();

	// starts the PL IP associated
	//start_ip(perf);

	XTime_GetTime(&timer_start);
	*ptr = timer_start;
	data1.lower_32 = 0;
	data1.upper_32 = 0;

	//
	//
	// Send data
//	for(int w = 0; w < loops; w++)
	for(int k = 0; k < loops; k++)
	for(int j = 0; j < loops; j++)
	for(int i = 0; i < loops; i++)
	{
	    if(test == pr_flow::CIRCULAR_2CORE_128){
	    	Stream1.write(data1);
	    }
	    else if(test == pr_flow::CIRCULAR_2CORE){
	    	Stream1.primitive_write(data);
	    }
	    data1.lower_32++;
	    data1.mid_lo_32++;
	    data1.mid_hi_32++;
	    data1.upper_32++;
	    data++;
//			works
//			if(rand() % 15 == 9)
//			{
//				usleep( (rand()%20) );
//			}


	}

	//
	//
	// destroy streams
	Stream1.~stream();

	synchronize();

	return XST_SUCCESS;
}

// only a 2core example
int run_benchmark_2core_pl( pr_flow::test_t test, pr_flow::memory_t memory, pr_flow::axi_port_t port, pr_flow::stream_id_t id,pr_flow::width_t width )
{
	int loops = 100;
	XTime timer_start;
	uint64_t data = 0;
	pr_flow::wide_t data1;


	volatile XTime* ptr = (volatile XTime*)TIMER;
	XTime_StartTimer();

	Xil_SetTlbAttributes((UINTPTR)ptr, NORM_NONCACHE);
	dmb();

	//
	//
	// Create rx stream
	pr_flow::benchmark_stream Stream1( id, pr_flow::direction_t::TX,width,port,memory );

	Stream1.stream_init( test,memory );

	synchronize();

	XTime_GetTime(&timer_start);
	*ptr = timer_start;

	data1.lower_32=0;
	data1.upper_32=0;
	data = 0;

	//
	//
	// Send data
	for(int w = 0; w < loops; w++)
	for(int k = 0; k < loops; k++)
	for(int j = 0; j < loops; j++)
	for(int i = 0; i < loops; i++)
	{
		    if(test == pr_flow::CIRCULAR_2CORE_128){
		    	Stream1.write(data1);
		    }
		    else if(test == pr_flow::CIRCULAR_2CORE){
		    	Stream1.primitive_write(data);
		    }
		    else if(test == pr_flow::PRESENCE_2CORE){
		    	Stream1.presence_write(data);
		    }

		    data1.lower_32++;
		    data1.upper_32++;
		    data++;

//			to ensure works for variable delays
//			if(rand() % 15 == 11)
//			{
//				usleep( (rand()%20) );
//			}


	}

	XTime timer_end;
	XTime_GetTime(&timer_end);

#ifdef RUNNING_R5
	timer_start = *ptr;
	uint64_t bits = loops * loops *loops * loops * sizeof(uint64_t); // bits
	double seconds = ((double)(timer_end - timer_start) / (COUNTS_PER_SECOND / 1000000)); // useconds
	double tput = (bits/seconds); // b/us ->gbps
	tput /= 1000;
	tput *= 8;
	printf("%s %s Throughput ~ %f gbps \n",Stream1.test_type_to_str(test),Stream1.mem_type_to_str(memory),tput);
#endif

	synchronize();

	Stream1.stop_ip();
	//
	//
	// destroy streams
	Stream1.~stream();


	return XST_SUCCESS;
}

// only a 2core example
int run_benchmark_2core_ps( pr_flow::test_t test, pr_flow::memory_t memory, pr_flow::axi_port_t port )
{
	int loops = 100;
	XTime timer_start;
	uint64_t data = 0;
	XTime timer_end;
	pr_flow::wide_t data1;

	data1.lower_32=0;
	data1.upper_32=0;
	data = 0;

	volatile XTime* ptr = (volatile XTime*)TIMER;
	XTime_StartTimer();

	Xil_SetTlbAttributes((UINTPTR)ptr, NORM_NONCACHE);
	dmb();

//	XTime_GetTime(&timer_start);
//   sleep(4);
//   XTime_GetTime(&timer_end);
//
//   double t = ((double)(timer_end - timer_start) / (COUNTS_PER_SECOND)); // useconds
//   printf("sync time A53 ~ %fs \n",t);

	//
	//
	// Create rx stream
	pr_flow::benchmark_stream Stream1( pr_flow::stream_id_t::STREAM_ID_0, pr_flow::direction_t::SW_SHARED,pr_flow::width_t::SW_WIDTH,port,memory );

	synchronize();

	Stream1.start_stream();

	XTime_GetTime(&timer_start);
	*ptr = timer_start;

	//
	//
	// Send data
	for(int w = 0; w < loops; w++)
	for(int k = 0; k < loops; k++)
	for(int j = 0; j < loops; j++)
	for(int i = 0; i < loops; i++)
	{
	    if(test == pr_flow::CIRCULAR_2CORE_128){
	    	Stream1.write(data1);
	    }
	    else if(test == pr_flow::CIRCULAR_2CORE){
	    	Stream1.primitive_write(data);
	    }
	    else if(test == pr_flow::PRESENCE_2CORE){
	    	Stream1.presence_write(data);
	    }

	    data1.lower_32++;
	    data1.upper_32++;
	    data++;

//			add delay to check
//			if(rand() % 15 == 11)
//			{
//				usleep( (rand()%20) );
//			}


	}

	XTime_GetTime(&timer_end);

#ifdef RUNNING_R5
	timer_start = *ptr;
	uint64_t bits = loops * loops *loops * loops * sizeof(uint64_t); // bits
	double seconds = ((double)(timer_end - timer_start) / (COUNTS_PER_SECOND / 1000000)); // useconds
	double tput = (bits/seconds); // b/us ->gbps
	tput /= 1000;
	tput *= 8;
	printf("%s %s Throughput ~ %f gbps \n",Stream1.test_type_to_str(test),Stream1.mem_type_to_str(memory),tput);
#endif
	//
	//
	// destroy streams
	Stream1.~stream();

	synchronize();

	return XST_SUCCESS;
}

