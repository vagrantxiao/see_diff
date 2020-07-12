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
#include "assert.h"
#include "../Overlays/mmu.h"

void delay(int delay)
{
	for(int i = 0; i < delay; i++)
	{
		// spin
	}
}

int run_benchmark_ps_ps_4core( pr_flow::test_t test, pr_flow::memory_t memory, pr_flow::axi_port_t port  )
{
	int loops = 100;
	uint64_t data = 0;
	pr_flow::wide_t data1;
	//
	//
	// Create rx stream
	pr_flow::benchmark_stream Stream_in( pr_flow::stream_id_t::STREAM_ID_0, pr_flow::direction_t::SW_SHARED,pr_flow::width_t::SW_WIDTH, port,memory );

	pr_flow::benchmark_stream Stream_out( pr_flow::stream_id_t::STREAM_ID_1, pr_flow::direction_t::SW_SHARED,pr_flow::width_t::SW_WIDTH, port,memory );

	synchronize();

	//
	//
	// Send data
	for(int w = 0; w < loops; w++)
	for(int k = 0; k < loops; k++)
	for(int j = 0; j < loops; j++)
	for(int i = 0; i < loops; i++)
	{
	    if(test == pr_flow::CIRCULAR_2CORE_128){
	    	Stream_in.read(&data1);
	    	Stream_out.write(data1);
	    }
	    else if(test == pr_flow::CIRCULAR_2CORE){
	    	data = Stream_in.primitive_read();
	    	Stream_out.primitive_write(data);
	    }

	}

	//
	//
	// destroy streams
	Stream_in.~stream();
	Stream_out.~stream();

	synchronize();

	return XST_SUCCESS;
}

int run_benchmark_ps_pl_4core( pr_flow::test_t test, pr_flow::memory_t memory, pr_flow::axi_port_t port, pr_flow::width_t width  )
{
	int loops = 100;
	uint64_t data = 0;
	pr_flow::wide_t data1;
	//
	//
	// Create rx stream
	pr_flow::benchmark_stream Stream_in( pr_flow::stream_id_t::STREAM_ID_0, pr_flow::direction_t::RX,width, port, memory );

	pr_flow::benchmark_stream Stream_out( pr_flow::stream_id_t::STREAM_ID_1, pr_flow::direction_t::TX,width, port,memory );

	synchronize();

	Stream_in.start_stream();

	//
	//
	// Send data
//	for(int w = 0; w < loops; w++)
	for(int k = 0; k < loops; k++)
	for(int j = 0; j < loops; j++)
	for(int i = 0; i < loops; i++)
	{
	    if(test == pr_flow::CIRCULAR_2CORE_128){
	    	Stream_in.read(&data1);
	    	Stream_out.write(data1);
	    }
	    else if(test == pr_flow::CIRCULAR_2CORE){
	    	data = Stream_in.primitive_read();
	    	Stream_out.primitive_write(data);
	    }
	}

	//
	//
	// destroy streams
	Stream_in.~stream();
	Stream_out.~stream();

	synchronize();

	return XST_SUCCESS;
}

