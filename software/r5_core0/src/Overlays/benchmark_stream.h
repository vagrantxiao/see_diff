/*
 * benchmark_stream.h
 *
 *  Created on: Apr 10, 2020
 *      Author: emica
 */

#ifndef SRC_OVERLAYS_BENCHMARK_STREAM_H_
#define SRC_OVERLAYS_BENCHMARK_STREAM_H_

#include "stream.h"

#define PRESENCE_BIT ( 0x100000000 ) // 1 << 32
#define MASK32 (0xFFFFFFFF) // (1 << 32) - 1

namespace pr_flow{

enum test_t{
	LOOP_BACK32 =0,
	LOOP_BACK64,
	LOOP_BACK128,
	READ_ONLY32,
	READ_ONLY64,
	READ_ONLY128,
	WRITE_ONLY32,
	WRITE_ONLY64,
	WRITE_ONLY128,
	PRESENCE_2CORE,
	CIRCULAR_2CORE,
	PS_MEMORY,
	MAX_TEST,
	INVALID_TEST = MAX_TEST,
};

typedef struct Bsp_Template{
    u16 DeviceId;
    u32 Control_BaseAddress;
} bsp_template_t;

//
class benchmark_stream : public stream{

public:


	// constructor / destructor
	benchmark_stream( stream_id_t id, direction_t direction, axi_port_t port ) : stream(id,direction,port) {}
	~benchmark_stream();

	//stream initialization
	int stream_init( test_t id, memory_t mem );

	// read and write for measuring performance
	uint32_t simple_read();
	void simple_write( uint32_t data );

	uint32_t presence_read();
	void presence_write( uint32_t data );

	// polling
	uint32_t is_stream_done();

//	void print_state();

	// start streams
	void start_stream();

	//
	const char* test_type_to_str( pr_flow::test_t test );
	test_t test_str_to_type( char* str );
	int16_t get_width();
	void stop_ip();
	void print_state();

private:

	test_t test;

	void* axi_config_tx;
	void* axi_config_rx;

	int initialize_ip();
	void set_ptrs( u32 offset );

};

// # TODO make a compile time map for the IP's
//
// #ifdef XEXAMPLE_IP
// #define IP XEXAMPLE_IP
// #else
// #define IP 0
//

};

#endif /* SRC_OVERLAYS_BENCHMARK_STREAM_H_ */
