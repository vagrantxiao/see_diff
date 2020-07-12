#ifndef BENCHMARK_H
#define BENCHMARK_H

#include "../Overlays/stream.h"
#include "../Overlays/benchmark_stream.h"

int run_benchmark_ps_ps_4core( pr_flow::test_t test, pr_flow::memory_t memory, pr_flow::axi_port_t port  );

/*
* Description: instantiates a stream to do core to core communication using dedeicated
* IP used for benchmarking algorithms as well as testing data is relayed correctly.
* @param memory; type of memory to create a stream, DDR,OCM,CACHE
* @param port; what port IP is tied to, solely for book keeping
*/
int run_benchmark_ps_pl_4core( pr_flow::test_t test, pr_flow::memory_t memory, pr_flow::axi_port_t port, pr_flow::width_t width  );


int setup_mutex();
void synchronize();
void get_lock();
void release_lock();

#endif
