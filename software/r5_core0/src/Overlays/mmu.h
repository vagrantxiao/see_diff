#ifndef _MY_MMU_H_
#define _MY_MMU_H_

#define NORM_NONCACHE 0x401UL 	/* Normal Non-cacheable*/

#include "stream.h"

/*
* configures our IP and all other start up routines
*/ 
void init();

/*
* @param stream_id: id associated with an IP this along with direction dictates the memory region it reads or writes to.
* @param driection: does this stream read or write. This dictates where it will read or write to in memory.
*/
volatile uint64_t get_memory( pr_flow::stream_id_t stream_id, pr_flow::direction_t direction );


/*
* Description: simple synchronize function that waits for all 4 cores 
* used to synchronize on test start and test end
*/
void synchronize();

/*
* Description: acquires mutex from IP
*/
void get_lock();

/*
* Description: releases mutex IP 
*/
void release_lock();

/*
*
*/
void start_ip();

/*
*
*/
void shutdown_ip();

#endif
