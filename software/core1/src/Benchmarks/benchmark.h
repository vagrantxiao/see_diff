#ifndef BENCHMARK_H
#define BENCHMARK_H

#include "../Overlays/stream.h"
#include "../Overlays/benchmark_stream.h"

/*
* Description: instantiates a stream to do core to core communication using a purely
* software streams. Each core reads and writes directly to each other
* used for benchmarking algorithms as well as testing data is relayed correctly.
* @param memory; type of memory to create a stream, DDR,OCM,CACHE
* @param port; what port IP is tied to, solely for book keeping
*/
int run_benchmark_ps_ps_4core( pr_flow::test_t test, pr_flow::memory_t memory, pr_flow::axi_port_t port );

/*
* Description: instantiates a stream to do core to core communication using dedeicated
* IP used for benchmarking algorithms as well as testing data is relayed correctly.
* @param memory; type of memory to create a stream, DDR,OCM,CACHE
* @param port; what port IP is tied to, solely for book keeping
*/
int run_benchmark_ps_pl_4core( pr_flow::test_t test, pr_flow::memory_t memory, pr_flow::axi_port_t port, pr_flow::width_t width  );


/*
* Description: instantiates a stream to do core to core communication using dedeicated
* IP used for benchmarking algorithms as well as testing data is relayed correctly.
* designed for specifically 2 cores
* @param memory; type of memory to create a stream, DDR,OCM,CACHE
* @param port; what port IP is tied to, solely for book keeping
*/
int run_benchmark_2core_pl( pr_flow::test_t test, pr_flow::memory_t memory, pr_flow::axi_port_t port, pr_flow::stream_id_t id );


/*
* Description: instantiates a stream to do core to core communication using a purely
* software streams. Each core reads and writes directly to each other
* designed for specifically 2 cores
* @param memory; type of memory to create a stream, DDR,OCM,CACHE
* @param port; what port IP is tied to, solely for book keeping
*/
int run_benchmark_2core_ps( pr_flow::test_t test, pr_flow::memory_t memory, pr_flow::axi_port_t port );

/*
* Description: sets up the mutex IP for each core
*/
int setup_mutex();

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


#endif
