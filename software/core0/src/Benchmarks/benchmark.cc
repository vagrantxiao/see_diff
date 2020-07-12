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
#include "../Overlays/mmu.h"
#include "assert.h"
#include "../Overlays/benchmark_stream.h"

#define TIMER (0xFFFEB0016) // shared memory region for creation of sw streams

int run_bert( pr_flow::test_t test, pr_flow::memory_t memory, pr_flow::axi_port_t port,pr_flow::stream_id_t id )
{
	XTime timer_start=0;
	XTime timer_end=0;

    //
    XTime_StartTimer();


	//
	//
	// Create tx Stream readonly
	pr_flow::benchmark_stream Input_Stream( id, pr_flow::direction_t::TX,pr_flow::U32_BITS,port,memory );

	//
	Input_Stream.stream_init( test,memory );

	// gets the BRAM buffer Addresses
	Input_Stream.get_bert_ptrs();


	// fill up the buffer to copy
	for(int i = 0; i < 10000; i++)
	{
		Input_Stream.write_bert_out(i);
	}

	// set variable to 0 to copy into temp in HLS IP
	Input_Stream.set_bert_vars(0);

	// start the stream
	Input_Stream.start_stream();

	//
	//
	// Wait for PL to finish
	while(1)
	{
		usleep(50);
		if(Input_Stream.is_stream_done())
			break;
	}

	// run stop command, for bert IP we just print some variable to check
	Input_Stream.stop_ip();

	// read back buffer, should be empty
	for(int i = 0; i < 10000; i++)
	{
		printf("%d \n",Input_Stream.read_bert_in());
	}


	// set bert to copy to out buffer
	Input_Stream.set_bert_vars(1);

	// start IP
	Input_Stream.start_stream();

	//
	//
	// Wait for PL to finish
	while(1)
	{
		usleep(50);
		if(Input_Stream.is_stream_done())
			break;
	}


	// read back values should contain the values we wrote to out previously
	for(int i = 0; i < 10000; i++)
	{
		printf("%d \n",Input_Stream.read_bert_in());
	}


	Input_Stream.stop_ip();

	//
	//
	// destroy streams
	Input_Stream.~stream();

	return XST_SUCCESS;
}

int run_consumer( pr_flow::memory_t memory, pr_flow::axi_port_t port,pr_flow::stream_id_t id )
{
	XTime timer_start=0;
	XTime timer_end=0;
	uint8_t loops = 100;
	pr_flow::wide_t data1;
	data1.lower_32=0;
	data1.mid_lo_32=0;
	data1.mid_hi_32=0;
	data1.upper_32=0;

    //
    //
    XTime_StartTimer();

	//
	//
	// Create tx Stream readonly
	pr_flow::benchmark_stream Input_Stream( id, pr_flow::direction_t::TX,pr_flow::U32_BITS,port,memory );
	pr_flow::benchmark_stream consumer( id, pr_flow::direction_t::RX,pr_flow::U32_BITS,port,memory );

	//
	Input_Stream.stream_init( pr_flow::test_t::CIRCULAR_2CORE_128,memory );
	consumer.stream_init( pr_flow::test_t::CONSUME,memory );

	synchronize();

	// start the stream
	consumer.start_stream();
	Input_Stream.start_stream();

	uint32_t expected=0;

	XTime_GetTime(&timer_start);


	for(int k = 0; k < loops; k++)
	for(int j = 0; j < loops; j++)
	for(int i = 0; i < loops; i++)
	{
		Input_Stream.write(data1);
		data1.lower_32++;
//		expected++;
    }
	//
	//
	// Wait for PL to finish
	while(1)
	{
		if(consumer.is_stream_done())
			break;
	}

	XTime_GetTime(&timer_end);

	assert(timer_end > timer_start);
	uint64_t bits = 100*100*100 *sizeof(pr_flow::wide_t); // bits
	double seconds = ((double)(timer_end - timer_start) / (COUNTS_PER_SECOND / 1000000)); // useconds
	double tput = (bits/seconds); // b/us ->gbps
	tput /= 1000;
	tput *= 8;

	get_lock();
	printf("PS GMEM WRITE PL GMEM READ %s %s Throughput ~  %.2lf gbps \n",Input_Stream.mem_type_to_str(memory),Input_Stream.port_type_to_str(port),tput);
	release_lock();

	// run stop command, for bert IP we just print some variable to check
	Input_Stream.stop_ip();

	//
	//
	// destroy streams
	Input_Stream.~stream();
	consumer.~stream();

	return XST_SUCCESS;
}

int run_producer( pr_flow::memory_t memory, pr_flow::axi_port_t port,pr_flow::stream_id_t id )
{
	XTime timer_start=0;
	XTime timer_end=0;
	uint8_t loops = 100;
	pr_flow::wide_t data1;
	data1.lower_32=0;

    //
    XTime_StartTimer();

	//
	//
	// Create tx Stream readonly
	pr_flow::benchmark_stream Input_Stream( id, pr_flow::direction_t::RX,pr_flow::U32_BITS,port,memory );
	pr_flow::benchmark_stream producer( id, pr_flow::direction_t::TX,pr_flow::U32_BITS,port,memory );

	//
	Input_Stream.stream_init( pr_flow::test_t::CIRCULAR_2CORE_128,memory );
	producer.stream_init( pr_flow::test_t::PRODUCE,memory );

	synchronize();

	// start the stream
	Input_Stream.start_stream();
	producer.start_stream();

	uint32_t expected=0;

	XTime_GetTime(&timer_start);


	for(int k = 0; k < loops; k++)
	for(int j = 0; j < loops; j++)
	for(int i = 0; i < loops; i++)
	{
		Input_Stream.read(&data1);
//		if(expected != data1.lower_32 )
//		{
//			printf("Expected: %d received: %d \n",expected,data1.lower_32);
//			assert(expected == data1.lower_32);
//		}
		expected++;
    }
	//
	//
	// Wait for PL to finish
	while(1)
	{
		if(producer.is_stream_done())
			break;
	}

	XTime_GetTime(&timer_end);

	assert(timer_end > timer_start);
	uint64_t bits = 100*100*100 *sizeof(pr_flow::wide_t); // bits
	double seconds = ((double)(timer_end - timer_start) / (COUNTS_PER_SECOND / 1000000)); // useconds
	double tput = (bits/seconds); // b/us ->gbps
	tput /= 1000;
	tput *= 8;

	get_lock();
	printf("PS GMEM WRITE PL GMEM READ %s %s Throughput ~  %.2lf gbps \n",Input_Stream.mem_type_to_str(memory),Input_Stream.port_type_to_str(port),tput);
	release_lock();


	// run stop command, for bert IP we just print some variable to check
	Input_Stream.stop_ip();

	//
	//
	// destroy streams
	Input_Stream.~stream();
	producer.~stream();

	return XST_SUCCESS;
}

/// runs the standalone PL tests
int run_benchmark_pl( pr_flow::test_t test, pr_flow::memory_t memory, pr_flow::axi_port_t port,pr_flow::stream_id_t id )
{
	u32 Metrics;
	u32 ClkCntHigh = 0x0;
	u32 ClkCntLow = 0x0;
	int Status;
	int slot=0;

	XTime timer_start=0;
	XTime timer_end=0;

    uint8_t width = 0;


    //
    XTime_StartTimer();


	//
	//
	// Create tx Stream readonly
	pr_flow::benchmark_stream Input_Stream( id, pr_flow::direction_t::TX,pr_flow::U32_BITS, port,memory );

	//
	//
	// Create rx stream write only
	pr_flow::benchmark_stream Output_Stream( id, pr_flow::direction_t::RX,pr_flow::U32_BITS, port,memory );



	// test is one way so instantiate and start IP for each case
	if( test == pr_flow::READ_ONLY32 || test == pr_flow::READ_ONLY64 || test == pr_flow::READ_ONLY128  || test == pr_flow::READ_ONLY128_BURST )
	{
		Input_Stream.stream_init( test,memory );
		Input_Stream.start_stream();
		width = Input_Stream.get_width(test);
	}
	else if( test == pr_flow::WRITE_ONLY32 || test == pr_flow::WRITE_ONLY64 || test == pr_flow::WRITE_ONLY128  || test == pr_flow::WRITE_ONLY128_BURST )
	{
		Output_Stream.stream_init( test, memory );
		width = Output_Stream.get_width(test);
		Output_Stream.start_stream();
	}
	// loop back tests require a read and writer
	else
	{
		Input_Stream.stream_init( test,memory );
		Output_Stream.stream_init( test, memory );

		width = Output_Stream.get_width(test);
		Input_Stream.start_stream();
		Output_Stream.start_stream();
	}

	XTime_GetTime(&timer_start);


	//
	//
	// Wait for PL to finish
	while(1)
	{
		usleep(50);

		// read only has just the input stream so we will poll this function
		if( test == pr_flow::READ_ONLY32 || test == pr_flow::READ_ONLY64 || test == pr_flow::READ_ONLY128 || test == pr_flow::READ_ONLY128_BURST )
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

	XTime_GetTime(&timer_end);

	assert(timer_end > timer_start);
	uint64_t bits = 8000000 *width; // bits
	double seconds = ((double)(timer_end - timer_start) / (COUNTS_PER_SECOND / 1000000)); // useconds
	double tput = (bits/seconds); // b/us ->gbps
	tput /= 1000;
	tput *= 8;
	printf("%s %s %s Throughput ~  %.2lf gbps \n",Output_Stream.test_type_to_str(test),Output_Stream.mem_type_to_str(memory),Output_Stream.port_type_to_str(port),tput);

	//
	//
	// Read data and confirm 
	// useful for coherency checks we should see something for loopback tests
//	if( test == pr_flow::LOOP_BACK32 || test == pr_flow::LOOP_BACK64 || test == pr_flow::LOOP_BACK128 ){
//	for(int i = 0; i < pr_flow::RAW_BUFFER_SIZE; i++)
//	{
//		printf("simple_read(%d)\n",Output_Stream.simple_read());
//		sleep(1);
//	}
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
	XTime timer_start=0;
	XTime timer_end=0;
    int loops = 100;
    uint64_t temp[200];

    XTime_StartTimer();

	//
	//
	//printf("Format: Memory_Config,Total_Read_Latency,Bytes_Read... Running Test with buffer size %d and rest time of %d\n",buffer_size,time);

	//
	//
	// Create tx Stream
    pr_flow::benchmark_stream Input_Stream( pr_flow::stream_id_t::STREAM_ID_0, pr_flow::direction_t::SW_SHARED,pr_flow::U32_BITS, port,memory );
    Input_Stream.stream_init( pr_flow::test_t::PS_MEMORY,memory );


	XTime_GetTime(&timer_start);

	for(int k = 0; k < loops; k++)
	for(int j = 0; j < loops; j++)
	for(int i = 0; i < loops; i++)
	{
		Input_Stream.inline_vector_write(i);
	}

	XTime_GetTime(&timer_end);

	assert(timer_end > timer_start);
	uint64_t bits = loops *loops * loops * sizeof(uint64_t)*4; // bits
	double seconds = ((double)(timer_end - timer_start) / (COUNTS_PER_SECOND / 1000000)); // useconds
	double tput = (bits/seconds); // b/us ->gbps
	tput /= 1000;
	tput *= 8;
	printf("PS VECTOR WRITE %s Throughput ~ %f gbps \n",Input_Stream.mem_type_to_str(memory),tput);

	uint64_t out_data[4];

	XTime_GetTime(&timer_start);
	//
	//
	// Read data and confirm
//	for(int z = 0; z < loops; z++)
	for(int k = 0; k < loops; k++)
	for(int j = 0; j < loops; j++)
	for(int i = 0; i < loops; i++)
	{
		Input_Stream.inline_vector_read( &out_data[0] );
	}

	XTime_GetTime(&timer_end);

	assert(timer_end > timer_start);

	bits = loops *loops * loops * sizeof(uint64_t)*4; // bits
	seconds = ((double)(timer_end - timer_start) /(COUNTS_PER_SECOND / 1000000)); // useconds
	tput = (bits/seconds); // b/us ->gbps
	tput /= 1000;
	tput *= 8;
	printf("PS VECTOR READ %s Throughput ~ %f gbps \n",Input_Stream.mem_type_to_str(memory),tput);


    XTime_GetTime(&timer_start);

	//
	//
	// perform simple memory writes
	// write, increment, write etc...
//	for(int z = 0; z < loops; z++)
	for(int k = 0; k < loops; k++)
	for(int j = 0; j < loops; j++)
	for(int i = 0; i < loops; i++)
	{
		Input_Stream.simple_write(i);
	}

	XTime_GetTime(&timer_end);

	assert(timer_end > timer_start);

	bits = loops *loops * loops * sizeof(uint64_t); // bits
	seconds = ((double)(timer_end - timer_start) / (COUNTS_PER_SECOND / 1000000)); // useconds
	tput = (bits/seconds); // b/us ->gbps
	tput /= 1000;
	tput *= 8;
	printf("PS SIMPLE WRITE %s Throughput ~ %f gbps \n",Input_Stream.mem_type_to_str(memory),tput);

	XTime_GetTime(&timer_start);
	//
	//
	// Read data and confirm
//	for(int z = 0; z < loops; z++)
	for(int k = 0; k < loops; k++)
	for(int j = 0; j < loops; j++)
	for(int i = 0; i < loops; i++)
	{
		Input_Stream.simple_read();
	}

	XTime_GetTime(&timer_end);

	assert(timer_end > timer_start);

	bits = loops *loops * loops * sizeof(uint64_t); // bits
	seconds = ((double)(timer_end - timer_start) /(COUNTS_PER_SECOND / 1000000)); // useconds
	tput = (bits/seconds); // b/us ->gbps
	tput /= 1000;
	tput *= 8;
	printf("PS SIMPLE READ %s Throughput ~ %f gbps \n",Input_Stream.mem_type_to_str(memory),tput);

    XTime_GetTime(&timer_start);

	//
	//
	// perform simple memory writes
	// write, increment, write etc...
	for(int k = 0; k < loops; k++)
	for(int j = 0; j < loops; j++)
	for(int i = 0; i < loops; i++)
	{
		Input_Stream.unrolled_write(i);
	}

	XTime_GetTime(&timer_end);

	assert(timer_end > timer_start);

	bits = 32* loops *loops * loops * sizeof(uint64_t); // bits
	seconds = ((double)(timer_end - timer_start) / (COUNTS_PER_SECOND / 1000000)); // useconds
	tput = (bits/seconds); // b/us ->gbps
	tput /= 1000;
	tput *= 8;
	printf("PS UNROLLED FACTOR 32 WRITE %s Throughput ~ %f gbps \n",Input_Stream.mem_type_to_str(memory),tput);


	XTime_GetTime(&timer_start);
	//
	//
	// Read data and confirm
	for(int k = 0; k < loops; k++)
	for(int j = 0; j < loops; j++)
	for(int i = 0; i < loops; i++)
	{
		Input_Stream.unrolled_read(&temp[0]);
	}

	XTime_GetTime(&timer_end);

	assert(timer_end > timer_start);

	bits = 32* loops *loops * loops * sizeof(uint64_t); // bits
	seconds = ((double)(timer_end - timer_start) /(COUNTS_PER_SECOND / 1000000)); // useconds
	tput = (bits/seconds); // b/us ->gbps
	tput /= 1000;
	tput *= 8;
	printf("PS UNROLLED FACTOR 32 READ %s Throughput ~ %f gbps \n",Input_Stream.mem_type_to_str(memory),tput);
//
    XTime_GetTime(&timer_start);

	//
	//
	// perform simple memory writes
	// write, increment, write etc...
//	for(int z = 0; z < loops; z++)
	for(int k = 0; k < loops; k++)
	for(int j = 0; j < loops; j++)
	for(int i = 0; i < loops; i++)
	{
		Input_Stream.inline_simple_write(i);
	}

	XTime_GetTime(&timer_end);

	assert(timer_end > timer_start);
	bits = loops *loops * loops * sizeof(uint64_t); // bits
	seconds = ((double)(timer_end - timer_start) / (COUNTS_PER_SECOND / 1000000)); // useconds
	tput = (bits/seconds); // b/us ->gbps
	tput /= 1000;
	tput *= 8;
	printf("PS INLINE WRITE %s Throughput ~ %f gbps \n",Input_Stream.mem_type_to_str(memory),tput);

	XTime_GetTime(&timer_start);

	uint64_t tempa = 0;
	//
	//
	// Read data and confirm
//	for(int z = 0; z < loops; z++)
	for(int k = 0; k < loops; k++)
	for(int j = 0; j < loops; j++)
	for(int i = 0; i < loops; i++)
	{
		tempa = Input_Stream.inline_simple_read();
	}

	XTime_GetTime(&timer_end);

	assert(timer_end > timer_start);

	bits = loops *loops * loops * sizeof(uint64_t); // bits
	seconds = ((double)(timer_end - timer_start) /(COUNTS_PER_SECOND / 1000000)); // useconds
	tput = (bits/seconds); // b/us ->gbps
	tput /= 1000;
	tput *= 8;
	printf("PS INLINE READ %s Throughput ~ %f gbps \n",Input_Stream.mem_type_to_str(memory),tput);


	//
	//
	// destroy streams
	Input_Stream.~stream();
	//Input_Stream2.~stream();

	return XST_SUCCESS;
}

// only a 2core example
int run_benchmark_ps_ps_4core( pr_flow::test_t test, pr_flow::memory_t memory, pr_flow::axi_port_t port )
{
	int loops = 100;
	XTime timer_end;
	XTime timer_start;
	uint64_t data =0;
	pr_flow::wide_t data1;
	volatile u32* ptr = (volatile u32*)TIMER;
	Xil_SetTlbAttributes((UINTPTR)ptr, NORM_NONCACHE);
	dmb();

	XTime_StartTimer();

	//
	//
	// Create rx stream
	pr_flow::benchmark_stream Stream1( pr_flow::stream_id_t::STREAM_ID_2, pr_flow::direction_t::SW_SHARED,pr_flow::U32_BITS,port,memory );

	// synchronizing for SW streams is important as we zero out the memory we do not want to have one core running faster and writing data and then
	// the other core zeros it out.
	synchronize();

	// note how we do not start the stream like we do in the PL version this is because we do not need to send any start signals to the IP

	uint64_t expected = 0;
	XTime_GetTime(&timer_start);
	//
	//
	// Send data
	for(int w = 0; w < loops; w++)
	for(int k = 0; k < loops; k++)
	for(int j = 0; j < loops; j++)
	for(int i = 0; i < loops; i++)
	{
		    if(test == pr_flow::CIRCULAR_2CORE_128){
		    	Stream1.read(&data1);
				if(expected != data1.lower_32 && data1.upper_32 != expected)
				{
					printf("Expected: %d received: %d \n",expected,data1.lower_32);
					assert(expected == data1.lower_32);
				}
		    }
		    if(test == pr_flow::CIRCULAR_2CORE){
		    	data = Stream1.primitive_read();
				if(expected != data)
				{
					printf("Expected: %d received: %d \n",expected,data);
					assert(expected == data);
				}
		    }
			expected++;

//			make sure we can ad delays since in reality applications will vary in throughput
//			if(rand() % 15 == 11)
//			{
//				usleep( (rand()%20) );
//			}


	}

	XTime_GetTime(&timer_end);
	timer_start = *ptr;
	assert(timer_end > timer_start);
	uint64_t bits = loops * loops *loops * loops * sizeof(uint64_t)*2; // bits, note this overflows on 32 bit processor
	double useconds = ((double)(timer_end - timer_start) / (COUNTS_PER_SECOND / 1000000)); // useconds
	double tput = (bits/useconds); // b/us ->
	tput /= 1000; // GBps
	tput *= 8; // gbps
	printf("PS PS Throughput ~ %f gbps \n",tput);

	//
	//
	// destroy streams
	Stream1.~stream();

	synchronize();

	return XST_SUCCESS;
}

int run_benchmark_ps_pl_4core( pr_flow::test_t test, pr_flow::memory_t memory, pr_flow::axi_port_t port, pr_flow::width_t width  )
{
	int loops = 100;
	XTime timer_end;
	XTime timer_start;
	volatile XTime* ptr = (volatile XTime*)TIMER;
	Xil_SetTlbAttributes((UINTPTR)ptr, NORM_NONCACHE);
	dmb();
	//
	//
	// Create rx stream
	pr_flow::benchmark_stream Stream( pr_flow::stream_id_t::STREAM_ID_3,  pr_flow::direction_t::RX,width, port,memory );
	Stream.stream_init(test,memory);


//	pr_flow::benchmark_stream Streamtx(  pr_flow::stream_id_t::STREAM_ID_3,  pr_flow::direction_t::TX,width, port,memory );
//	Streamtx.stream_init(test,memory);

	pr_flow::benchmark_stream producer(  pr_flow::stream_id_t::STREAM_ID_0,  pr_flow::direction_t::TX,width, port,memory );
	producer.stream_init( pr_flow::test_t::PRODUCE,memory );
//
//	pr_flow::benchmark_stream Streamrx( pr_flow::stream_id_t::STREAM_ID_3,  pr_flow::direction_t::RX,width, port,memory );
//
//	pr_flow::benchmark_stream Streamtx2(  pr_flow::stream_id_t::STREAM_ID_4,  pr_flow::direction_t::TX,width, port,memory );
//
//	pr_flow::benchmark_stream Streamrx2(  pr_flow::stream_id_t::STREAM_ID_4,  pr_flow::direction_t::RX,width, port,memory );
//
//	pr_flow::benchmark_stream Streamtx3(  pr_flow::stream_id_t::STREAM_ID_5,  pr_flow::direction_t::TX,width, port,memory );
//
//	pr_flow::benchmark_stream Streamrx3(  pr_flow::stream_id_t::STREAM_ID_5,  pr_flow::direction_t::RX,width, port,memory );

	synchronize();

	Stream.start_stream();
//	Streamtx.start_stream();
	producer.start_stream();

	uint64_t expected = 0;
	uint64_t data = 0;
	pr_flow::wide_t data1;
	data1.lower_32=0;
	data1.mid_lo_32=0;
	data1.mid_hi_32=0;
	data1.upper_32=0;

	XTime_StartTimer();
	XTime_GetTime(&timer_start);
	//
	//
	// Send data
//	for(int w = 0; w < loops; w++)
	for(int k = 0; k < loops; k++)
	for(int j = 0; j < loops; j++)
	for(int i = 0; i < loops; i++)
	{
		    if(test == pr_flow::CIRCULAR_2CORE_128){
		    	Stream.read(&data1);
//		    	if( width == 1 )
//		    	{
//		    		if(expected != data1.lower_32)
//		    		{
//		    			printf("Expected: %d received: %d \n",expected,data1.lower_32);
//		    			assert(expected == data1.lower_32);
//		    		}
//		    	}
//		    	else if( width == 2 )
//		    	{
//		    		if(expected != data1.lower_32 || expected != data1.mid_lo_32 )
//		    		{
//		    			printf("Expected: %d received: %d \n",expected,data1.lower_32);
//		    			assert(expected == data1.lower_32);
//		    		}
//		    	}
//		    	else if( width == 4 )
//		    	{
//		    		if(expected != data1.lower_32 || expected != data1.mid_lo_32 || expected != data1.mid_hi_32 || expected != data1.upper_32 )
//		    		{
//		    			printf("Expected: %d received: %d \n",expected,data1.lower_32);
//		    			assert(expected == data1.lower_32);
//		    		}
//		    	}
		    }
//		    else if(test == pr_flow::CIRCULAR_2CORE){
//		    	data = Stream.primitive_read();
//				if(expected != data)
//				{
//					printf("Expected: %d received: %d \n",expected,data);
//					assert(expected == data);
//				}
//		    }
			expected++;

//			works
//			if(rand() % 15 == 11)
//			{
//				usleep( (rand()%20) );
//			}


	}


	XTime_GetTime(&timer_end);
	//timer_start = *ptr;

	assert(timer_end > timer_start);
	uint64_t bits =  loops *loops * loops * sizeof(uint64_t)*2; // bits
	double seconds = ((double)(timer_end - timer_start) / (COUNTS_PER_SECOND / 1000000)); // useconds
	double tput = (bits/seconds); // b/us ->gbps
	tput /= 1000; // convert to giga
	tput *= 8; // convert to bit
	printf("PL PS Throughput ~ %f gbps \n",tput);


	//
	//
	// destroy streams
	Stream.~stream();
//	Streamtx.~stream();
//	Streamrx.~stream();
//	Streamtx2.~stream();
//	Streamrx2.~stream();
//	Streamtx3.~stream();
//	Streamrx3.~stream();

	synchronize();

	return XST_SUCCESS;
}

// only a 2core example
int run_benchmark_2core_pl( pr_flow::test_t test, pr_flow::memory_t memory, pr_flow::axi_port_t port, pr_flow::stream_id_t id,pr_flow::width_t s_width )
{
	int loops = 100;

	volatile XTime* ptr = (volatile XTime*)TIMER;
	Xil_SetTlbAttributes((UINTPTR)ptr, NORM_NONCACHE);
	dmb();

	XTime timer_end;
	XTime timer_start;

	XTime_StartTimer();

	//
	uint16_t width = 0;

	//
	// Create rx stream
	pr_flow::benchmark_stream Stream1( id, pr_flow::direction_t::RX, s_width ,port,memory);


	Stream1.stream_init( test,memory );
	width = Stream1.get_width(test);


	synchronize();
	Stream1.start_stream();

	uint64_t expected = 0;
	uint64_t data = 0;
	pr_flow::wide_t data1;

	XTime_GetTime(&timer_start);


	//
	// Send data
	for(int w = 0; w < loops; w++)
	for(int k = 0; k < loops; k++)
	for(int j = 0; j < loops; j++)
	for(int i = 0; i < loops; i++)
	{
		    if(test == pr_flow::CIRCULAR_2CORE_128){
		    	Stream1.read(&data1);
				if(expected != data1.lower_32 && data1.lower_32 != expected)
				{
					printf("Expected: %d received: %d \n",expected,data1.lower_32);
					assert(expected == data1.lower_32);
				}
		    }
		    else if(test == pr_flow::CIRCULAR_2CORE){
		    	data = Stream1.primitive_read();
				if(expected != data)
				{
					printf("Expected: %d received: %d \n",expected,data);
					assert(expected == data);
				}
		    }
		    else if(test == pr_flow::PRESENCE_2CORE){
		    	data = Stream1.presence_read();
				if(expected != data)
				{
					printf("Expected: %d received: %d \n",expected,data);
					assert(expected == data);
				}
		    }

			expected++;

//			add some delay to test
//			if(rand() % 15 == 11)
//			{
//				usleep( (rand()%20) );
//			}


	}

	XTime_GetTime(&timer_end);
	//timer_start = *ptr;

	assert(timer_end > timer_start);
	uint64_t bits = loops * loops * loops * loops * width; // bits
	double seconds = ((double)(timer_end - timer_start) / (COUNTS_PER_SECOND / 1000000)); // useconds
	double tput = (bits/seconds); // b/us ->gbps
	tput /= 1000;
	tput *= 8;
	printf("PL %s %s Throughput ~ %f gbps \n",Stream1.test_type_to_str(test),Stream1.mem_type_to_str(memory),tput);

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
	XTime timer_end;
	XTime timer_start;
	pr_flow::wide_t data1;
	uint64_t expected = 0;
	uint64_t data = 0;

	volatile XTime* ptr = (volatile XTime*)TIMER;
	Xil_SetTlbAttributes((UINTPTR)ptr, NORM_NONCACHE);
	dmb();

	uint16_t width = 0;

	//
	//
	// Create rx stream
	pr_flow::benchmark_stream Stream1( pr_flow::stream_id_t::STREAM_ID_0, pr_flow::direction_t::SW_SHARED,pr_flow::SW_WIDTH,port,memory );

//	Stream1.stream_init( test,memory );
	width = Stream1.get_width( test );

	synchronize();

	XTime_StartTimer();

	XTime_GetTime(&timer_start);

	//
	//
	// Send data
//	for(int w = 0; w < loops; w++)
	for(int k = 0; k < loops; k++)
	for(int j = 0; j < loops; j++)
	for(int i = 0; i < loops; i++)
	{
	    if(test == pr_flow::CIRCULAR_2CORE_128){
	    	Stream1.read(&data1);
			if(expected != data1.lower_32 && data1.lower_32 != expected)
			{
				printf("Expected: %d received: %d \n",expected,data1.lower_32);
				assert(expected == data1.lower_32);
			}
	    }
	    if(test == pr_flow::CIRCULAR_2CORE){
	    	data = Stream1.primitive_read();
			if(expected != data)
			{
				printf("Expected: %d received: %d \n",expected,data);
				assert(expected == data);
			}
	    }
	    else if(test == pr_flow::PRESENCE_2CORE){
	    	data = Stream1.presence_read();
			if(expected != data)
			{
				printf("Expected: %d received: %d \n",expected,data);
				assert(expected == data);
			}
	    }

			expected++;

//			works
//			if(rand() % 15 == 11)
//			{
//				usleep( (rand()%20) );
//			}


	}

	XTime_GetTime(&timer_end);
//	timer_start = *ptr;

	assert(timer_end > timer_start);
	uint64_t bits = loops *loops * loops * width; // bits
	double seconds = ((double)(timer_end - timer_start) / (COUNTS_PER_SECOND / 1000000)); // useconds
	double tput = (bits/seconds); // b/us ->gbps
	tput /= 1000;
	tput *= 8;
	printf("PS %s %s Throughput ~ %f gbps \n",Stream1.test_type_to_str(test),Stream1.mem_type_to_str(memory),tput);

	//
	//
	// destroy streams
	Stream1.~stream();

	synchronize();

	return XST_SUCCESS;
}
