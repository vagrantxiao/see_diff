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

// my stuff
#include "../Overlays/mmu.h"
#include "assert.h"
#include "../Overlays/benchmark_stream.h"
#include "../timer.h"

#define TIMER (0xFFFE00016) // shared memory region for creation of sw streams

// runs the standalone PL tests
int run_benchmark_pl( pr_flow::test_t test, pr_flow::memory_t memory, pr_flow::axi_port_t port )
{
	u32 Metrics;
	u32 ClkCntHigh = 0x0;
	u32 ClkCntLow = 0x0;
	int Status;
	int slot=0;

//	XTime timer_start=0;
//    XTime timer_end=0;

    uint8_t width = 0;

    pr_flow::stream_id_t stream_id_tx = pr_flow::stream_id_t::INVALID_STREAM;
    pr_flow::stream_id_t stream_id_rx = pr_flow::stream_id_t::INVALID_STREAM;

    //
//    XTime_StartTimer();

    // IP instances 1 are all tied to HPC0 ports
    // IP instance 0 are all tied to HP ports
    if(port == pr_flow::HPC0)
    {
    	stream_id_tx = pr_flow::stream_id_t::STREAM_ID_HW_TX1;
    	stream_id_rx = pr_flow::stream_id_t::STREAM_ID_HW_RX1;
    }
    else
    {
    	stream_id_tx = pr_flow::stream_id_t::STREAM_ID_HW_TX0;
    	stream_id_rx = pr_flow::stream_id_t::STREAM_ID_HW_RX0;
    }

	//
	//
	// Create tx Stream readonly
	pr_flow::benchmark_stream Input_Stream( stream_id_tx, pr_flow::direction_t::TX, port );

	//
	//
	// Create rx stream write only
	pr_flow::benchmark_stream Output_Stream( stream_id_rx, pr_flow::direction_t::RX, port );

	// test is one way so instantiate and start IP for each case
	if( test == pr_flow::READ_ONLY32 || test == pr_flow::READ_ONLY64 || test == pr_flow::READ_ONLY128 )
	{
		Input_Stream.stream_init( test,memory );
		Input_Stream.start_stream();
		width = Input_Stream.get_width();
	}
	else if( test == pr_flow::WRITE_ONLY32 || test == pr_flow::WRITE_ONLY64 || test == pr_flow::WRITE_ONLY128 )
	{
		Output_Stream.stream_init( test, memory );
		width = Output_Stream.get_width();
		Output_Stream.start_stream();
	}
	// loop back tests require a read and writer
	else
	{
		Input_Stream.stream_init( test,memory );
		Output_Stream.stream_init( test, memory );
		width = Output_Stream.get_width();
		Input_Stream.start_stream();
		Output_Stream.start_stream();
	}

	//
	//
	// Send data

//	XTime_GetTime(&timer_start);

	//
	//
	// Wait for PL to finish
	while(1)
	{
		// read only has just the input stream so we will poll this function
		if( test == pr_flow::READ_ONLY32 || test == pr_flow::READ_ONLY64 || test == pr_flow::READ_ONLY128 )
		{
			if(Input_Stream.is_stream_done())
				break;
		}
		else
		{
			if(Output_Stream.is_stream_done())
				break;
		}
	}

//	XTime_GetTime(&timer_end);
//	float cycles = timer_end-timer_start;
//	float bytes = 8000000 * width;
//	float tput = (bytes/cycles) * 1.2;
//	printf("%s %s Throughput ~ %f GBps \n",Output_Stream.test_type_to_str(test),Output_Stream.mem_type_to_str(memory),tput);


	//
	//
	// Read data and confirm 
	// useful for coherency checks
//	for(int i = 0; i < buffer_size; i++)
//	{
//		printf("simple_read(%d)\n",simple_read( STREAM_ID_1 ));
//		sleep(1);
//	}

	//
	//
	// destroy streams
	Input_Stream.~stream();
	Output_Stream.~stream();


	return XST_SUCCESS;
}

int run_benchmark_memory( pr_flow::memory_t memory, pr_flow::axi_port_t port )
{
	int Status;
    u32 timer_start=0;
    u32 timer_end=0;

    int loops = 100;

    StartTimer();

	//
	//
	//printf("Format: Memory_Config,Total_Read_Latency,Bytes_Read... Running Test with buffer size %d and rest time of %d\n",buffer_size,time);

	//
	//
	// Create tx Stream
    pr_flow::benchmark_stream Input_Stream( pr_flow::stream_id_t::STREAM_ID_HW_TX0, pr_flow::direction_t::SW_SHARED, port );
    //Input_Stream.stream_init( pr_flow::test_t::PS_MEMORY,memory );
	//
	//
	// Create rx stream
    pr_flow::benchmark_stream Input_Stream2( pr_flow::stream_id_t::STREAM_ID_HW_RX0, pr_flow::direction_t::SW_SHARED, port );
    //Input_Stream2.stream_init( pr_flow::test_t::PS_MEMORY,memory );


	GetTime(&timer_start);
	//printf("time %d\n",timer_start);

	//
	//
	// perform simple memory writes 
	// write, increment, write etc...
	for(int k = 0; k < loops; k++)
	for(int k = 0; k < loops; k++)
	for(int j = 0; j < loops; j++)
	for(int i = 0; i < loops; i++)
	{
		Input_Stream.simple_write(i);
	}

	GetTime(&timer_end);
	//printf("time %d\n",timer_end);

	float cycles = timer_end-timer_start;
	float bytes = loops * loops *loops * loops * sizeof(uint64_t);
	float tput = (bytes/cycles) * 3;
	printf("PS WRITE %s Throughput ~ %f GBps \n",Input_Stream.mem_type_to_str(memory),tput);
////
//
	GetTime(&timer_start);
	//
	//
	// Read data and confirm
	for(int k = 0; k < loops; k++)
	for(int k = 0; k < loops; k++)
	for(int j = 0; j < loops; j++)
	for(int i = 0; i < loops; i++)
	{
		Input_Stream2.simple_read();
	}

	GetTime(&timer_end);
//
//
	cycles = timer_end-timer_start;
	bytes = loops * loops *loops * loops * sizeof(uint64_t);
	tput = (bytes/cycles) * 3;
	printf("PS READ %s Throughput ~ %f GBps \n",Input_Stream.mem_type_to_str(memory),tput);

	//
	//
	// destroy streams
	Input_Stream.~stream();
	Input_Stream2.~stream();

	StopTimer();

	return XST_SUCCESS;
}

// only a 2core example
int run_benchmark_ps_ps_flow( pr_flow::memory_t memory, pr_flow::axi_port_t port )
{
	int loops = 100;

//	volatile XTime* ptr = (volatile XTime*)TIMER;
//	Xil_SetTlbAttributes((UINTPTR)ptr, NORM_NONCACHE);
//	dmb();

	//
	//
	// Create rx stream
	pr_flow::benchmark_stream Stream1( pr_flow::stream_id_t::STREAM_ID_SW2, pr_flow::direction_t::SW_SHARED,port );

	// synchronizing for SW streams is important as we zero out the memory we do not want to have one core running faster and writing data and then
	// the other core zeros it out.
	synchronize();

	// note how we do not start the stream like we do in the PL version this is because we do not need to send any start signals to the IP

	uint64_t expected = 0;
	//
	//
	// Send data
	for(int w = 0; w < loops; w++)
	for(int k = 0; k < loops; k++)
	for(int j = 0; j < loops; j++)
	for(int i = 0; i < loops; i++)
	{
		    uint64_t data = Stream1.read();

		    //
			if(expected != data)
			{
				printf("Expected: %d received: %d \n",expected,data);
				assert(expected == data);
			}
			expected++;

//			make sure we can ad delays since in reality applications will vary in throughput
//			if(rand() % 15 == 11)
//			{
//				usleep( (rand()%20) );
//			}


	}

//	XTime timer_end;
//	XTime timer_start;
//	XTime_GetTime(&timer_end);
//
//	timer_start = *ptr;
//	float cycles = timer_end-timer_start;
//	float bytes = expected * sizeof(uint64_t);
//	float tput = (bytes/cycles) * 1.2;
//	printf("PS PS Throughput ~ %f GBps \n",tput);

	//
	//
	// destroy streams
	Stream1.~stream();

	synchronize();

	return XST_SUCCESS;
}

int run_benchmark_ps_pl_flow( pr_flow::memory_t memory, pr_flow::axi_port_t port )
{
	int loops = 100;

//	volatile XTime* ptr = (volatile XTime*)TIMER;
//	Xil_SetTlbAttributes((UINTPTR)ptr, NORM_NONCACHE);
//	dmb();
	//
	//
	// Create rx stream
	pr_flow::stream Stream( pr_flow::stream_id_t::STREAM_ID_HW_RX2,  pr_flow::direction_t::RX, port );

	pr_flow::stream Streamtx(  pr_flow::stream_id_t::STREAM_ID_HW_TX3,  pr_flow::direction_t::TX, port );

	pr_flow::stream Streamrx( pr_flow::stream_id_t::STREAM_ID_HW_RX3,  pr_flow::direction_t::RX, port );

	pr_flow::stream Streamtx2(  pr_flow::stream_id_t::STREAM_ID_HW_TX4,  pr_flow::direction_t::TX, port );

	pr_flow::stream Streamrx2(  pr_flow::stream_id_t::STREAM_ID_HW_RX4,  pr_flow::direction_t::RX, port );

	pr_flow::stream Streamtx3(  pr_flow::stream_id_t::STREAM_ID_HW_TX5,  pr_flow::direction_t::TX, port );

	pr_flow::stream Streamrx3(  pr_flow::stream_id_t::STREAM_ID_HW_RX5,  pr_flow::direction_t::RX, port );

	synchronize();

	start_ip();

	uint64_t expected = 0;
	//
	//
	// Send data
	for(int w = 0; w < loops; w++)
	for(int k = 0; k < loops; k++)
	for(int j = 0; j < loops; j++)
	for(int i = 0; i < loops; i++)
	{
		    uint64_t data = Stream.read();

//		    for(int z = 0; z < 1; z++)
//		    {
//		    	Streamtx.write(z);
//		    }
//		    for(int z = 0; z < 5; z++)
//		    {
//		    	Streamtx2.write(z);
//		    }
//		    for(int z1 = 0; z1 < 1; z1++)
//		    {
//		    	uint32_t tx = Streamrx.read();
//		    	tx++;
//		    }
//		    for(int z1 = 0; z1 < 5; z1++)
//		    {
//		    	uint32_t tx = Streamrx2.read();
//		    }




			if(expected != data)
			{
				printf("Expected: %d received: %d \n",expected,data);
				//Stream.print_state();
				//assert(expected == data);
			}
			expected++;

//			works
//			if(rand() % 15 == 11)
//			{
//				usleep( (rand()%20) );
//			}


	}

//	XTime timer_end;
//	XTime timer_start;
//	XTime_GetTime(&timer_end);
//
//	timer_start = *ptr;
//	float cycles = timer_end-timer_start;
//	float bytes = expected * sizeof(uint64_t);
//	float tput = (bytes/cycles) * 1.2;
//	printf("PL PS Throughput ~ %f GBps \n",tput);

	//
	//
	// destroy streams
	Stream.~stream();
	Streamtx.~stream();
	Streamrx.~stream();

	Streamtx2.~stream();
	Streamrx2.~stream();
	Streamtx3.~stream();
	Streamrx3.~stream();

	synchronize();

	shutdown_ip();

	return XST_SUCCESS;
}

// only a 2core example
int run_benchmark_2core_pl( pr_flow::test_t test, pr_flow::memory_t memory, pr_flow::axi_port_t port )
{
	int loops = 100;

//	volatile XTime* ptr = (volatile XTime*)TIMER;
//	Xil_SetTlbAttributes((UINTPTR)ptr, NORM_NONCACHE);
//	dmb();

	uint16_t width = 0;

	//
	//
	// Create rx stream
	pr_flow::benchmark_stream Stream1( pr_flow::stream_id_t::STREAM_ID_HW_RX0, pr_flow::direction_t::RX,port );

	Stream1.stream_init( test,memory );
	width = Stream1.get_width();

	synchronize();
	Stream1.start_stream();

	uint64_t expected = 0;
	uint64_t data = 0;
	//
	//
	// Send data
	for(int w = 0; w < loops; w++)
	for(int k = 0; k < loops; k++)
	for(int j = 0; j < loops; j++)
	for(int i = 0; i < loops; i++)
	{
		    if(test == pr_flow::CIRCULAR_2CORE){
		    	data = Stream1.read();
		    }
		    else if(test == pr_flow::PRESENCE_2CORE){
		    	data = Stream1.presence_read();
		    }


			if(expected != data)
			{
				printf("Expected: %d received: %d \n",expected,data);
//				Stream1.print_state();
				assert(expected == data);
			}
			expected++;

//			works
//			if(rand() % 15 == 11)
//			{
//				usleep( (rand()%20) );
//			}


	}

//	XTime timer_end;
//	XTime timer_start;
//	XTime_GetTime(&timer_end);
//
//	timer_start = *ptr;
//	float cycles = timer_end-timer_start;
//	float bytes = expected * width;
//	float tput = (bytes/cycles) * 1.2;
//	printf("%s %s Throughput ~ %f GBps \n",Stream1.test_type_to_str(test),Stream1.mem_type_to_str(memory),tput);

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

//	volatile XTime* ptr = (volatile XTime*)TIMER;
//	Xil_SetTlbAttributes((UINTPTR)ptr, NORM_NONCACHE);
//	dmb();

	uint16_t width = 0;

	//
	//
	// Create rx stream
	pr_flow::benchmark_stream Stream1( pr_flow::stream_id_t::STREAM_ID_SW0, pr_flow::direction_t::SW_SHARED,port );

	//Stream1.stream_init( test,memory );
	//width = Stream1.get_width();

	synchronize();

	uint64_t expected = 0;
	uint64_t data = 0;
	//
	//
	// Send data
	for(int w = 0; w < loops; w++)
	for(int k = 0; k < loops; k++)
	for(int j = 0; j < loops; j++)
	for(int i = 0; i < loops; i++)
	{
		    if(test == pr_flow::CIRCULAR_2CORE){
		    	data = Stream1.read();
		    }
		    else if(test == pr_flow::PRESENCE_2CORE){
		    	data = Stream1.presence_read();
		    }


			if(expected != data)
			{
				printf("Expected: %d received: %d \n",expected,data);
//				Stream1.print_state();
				assert(expected == data);
			}
			expected++;

//			works
//			if(rand() % 15 == 11)
//			{
//				usleep( (rand()%20) );
//			}


	}

//	XTime timer_end;
//	XTime timer_start;
//	XTime_GetTime(&timer_end);
//
//	timer_start = *ptr;
//	float cycles = timer_end-timer_start;
//	float bytes = expected * width;
//	float tput = (bytes/cycles) * 1.2;
//	printf("%s %s Throughput ~ %f GBps \n",Stream1.test_type_to_str(test),Stream1.mem_type_to_str(memory),tput);

	//
	//
	// destroy streams
	Stream1.~stream();

	synchronize();

	return XST_SUCCESS;
}
