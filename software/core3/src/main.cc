/******************************************************************************
*
* Copyright (C) 2009 - 2014 Xilinx, Inc.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* Use of the Software is limited solely to applications:
* (a) running on a Xilinx device, or
* (b) that interact with a Xilinx device through a bus or interconnect.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* XILINX  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
* OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* Except as contained in this notice, the name of the Xilinx shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Xilinx.
*
******************************************************************************/

/*
 * helloworld.c: simple test application
 *
 * This application configures UART 16550 to baud rate 9600.
 * PS7 UART (Zynq) is not initialized by this application, since
 * bootrom/bsp configures it to baud rate 115200
 *
 * ------------------------------------------------
 * | UART TYPE   BAUD RATE                        |
 * ------------------------------------------------
 *   uartns550   9600
 *   uartlite    Configurable only in HW design
 *   ps7_uart    115200 (configured by bootrom/bsp)
 */

#include <stdio.h>
#include "xil_printf.h"
#include "Benchmarks/benchmark.h"
#include "Overlays/stream.h"
#include "Overlays/benchmark_stream.h"
#include "Overlays/mmu.h"
#include "xtime_l.h"
#include "render.h"
#include "sleep.h"
#include "Overlays/user_configs.h"
#define TESTS 10

int main()
{
	sleep(5);
	init();

#if ACTIVE_CORES == 4
#ifdef RUN_BENCHMARK
    run_benchmark_ps_ps_4core( pr_flow::CIRCULAR_2CORE_128, pr_flow::RR_CACHE, pr_flow::ACP );
    run_benchmark_ps_ps_4core( pr_flow::CIRCULAR_2CORE_128, pr_flow::OCM, pr_flow::ACP );
    run_benchmark_ps_pl_4core( pr_flow::CIRCULAR_2CORE_128, pr_flow::RR_CACHE, pr_flow::ACP, pr_flow::width_t::U128_BITS );
    run_benchmark_ps_pl_4core( pr_flow::CIRCULAR_2CORE_128, pr_flow::OCM, pr_flow::ACP,pr_flow::width_t::U128_BITS  );
#endif
#ifdef RUN_RENDER
    render_pl_mix( pr_flow::RR_CACHE );
//    render_pl_mix(pr_flow::OCM );
//    render_ps( pr_flow::RR_CACHE );
//    render_ps( pr_flow::OCM );
#endif
#endif

    return 0;
}
