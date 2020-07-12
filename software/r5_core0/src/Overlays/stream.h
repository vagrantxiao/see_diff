/*
 * base_stream.h
 *
 *  Created on: Apr 10, 2020
 *      Author: emica
 */

#ifndef SRC_OVERLAYS_STREAM_H_
#define SRC_OVERLAYS_STREAM_H_

#include <stdint.h>

#include "xcirc_buff_read_many.h" // reads from ocm
#include "xcirc_buff_write_many.h" // writes to ocm
#include "user_configs.h"


namespace pr_flow{

//
// enums / typedefs / macros
enum direction_t
{
	TX = 0,
	RX = 1,
	SW_SHARED,
};

enum memory_t
{
	DDR = 0, // data stored in DDR
	OCM = 1, // use OCM
	CACHE = 2,
	INVALID_MEMORY,
	MAX_NUM_MEMORIES = INVALID_MEMORY,
};

enum axi_port_t
{
	HPC0 = 0,
	HPC1,
	HP0,
	HP1,
	HP2,
	HP3,
	ACE,
	ACP,
	SW_PORT,
	INVALID_PORT,
	MAX_NUM_PORTS = INVALID_PORT,
};

enum meta_data_t
{
	POW_2 = BUFFER_SIZE_POW_2,
	RAW_BUFFER_SIZE = POW_2+2,
	HEAD_POINTER = POW_2+1,
	TAIL_POINTER = POW_2,
	MASK = POW_2-1,
};

// convert this to rx and tx enums
enum stream_id_t{
	STREAM_ID_HW_TX0 = 0,
	STREAM_ID_HW_RX0 = 0,
	STREAM_ID_HW_TX1 = 1,
	STREAM_ID_HW_RX1 = 1,
	STREAM_ID_HW_TX2 = 2,
	STREAM_ID_HW_RX2 = 2,
	STREAM_ID_HW_TX3 = 3,
	STREAM_ID_HW_RX3 = 3,
	STREAM_ID_HW_TX4 = 4,
	STREAM_ID_HW_RX4 = 4,
	STREAM_ID_HW_TX5 = 5,
	STREAM_ID_HW_RX5 = 5,
	STREAM_ID_HW_TX6 = 6,
	STREAM_ID_HW_RX6 = 6,
	STREAM_ID_HW_TX7 = 7,
	STREAM_ID_HW_RX7 = 7,
	STREAM_ID_HW_TX8 = 8,
	STREAM_ID_HW_RX8 = 8,
	STREAM_ID_HW_TX9 = 9,
	STREAM_ID_HW_RX9 = 9,
	STREAM_ID_SW0 = 0,
	STREAM_ID_SW1 = 1,
	STREAM_ID_SW2 = 2,
	STREAM_ID_SW3 = 3,
	STREAM_ID_SW4 = 4,
	STREAM_ID_SW5 = 5,
	STREAM_ID_SW6 = 6,
	STREAM_ID_SW7 = 7,
	STREAM_ID_SW8 = 8,
	STREAM_ID_SW9 = 9,
	STREAM_ID_SW10 = 10,
	MAX_NUMBER_OF_STREAMS =20,
	INVALID_STREAM = MAX_NUMBER_OF_STREAMS,
};

class stream{

public:

	stream( stream_id_t id, direction_t direction, axi_port_t port );
	~stream();

	// circular buffer implementation for sending data to PL
	void write( uint64_t data );
	uint64_t read();

//	void print_state();

	// string to enum + enum to string conversions
	//
	const char* port_type_to_str( pr_flow::axi_port_t port );
	pr_flow::axi_port_t port_str_to_type( char* str );
	const char* mem_type_to_str( pr_flow::memory_t memory );
	pr_flow::memory_t mem_str_to_type( char* str );


protected:

	//
	// enums / typedefs / macros


	//
	// function defintions


	// private variables
	volatile uint64_t* buff;

	uint16_t ptr;

	//
	uint32_t buff_size;

	// head and tail pointers
	uint16_t head;
	uint16_t tail;

	//
	uint32_t bytes_read;
	uint32_t bytes_written;

	axi_port_t port;
	memory_t memory;
	direction_t direction;

	uint32_t bsp_id;
	stream_id_t stream_id;



};

};


#endif /* SRC_OVERLAYS_STREAM_H_ */
