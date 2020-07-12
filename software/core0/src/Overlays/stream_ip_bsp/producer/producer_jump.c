/*
 * producer_jump.c
 *
 *  Created on: Jun 18, 2020
 *      Author: emica
 */
#include "../../user_configs.h"
#include "../bsp_jump_table.h"
#include <stdint.h>
#include <stdlib.h>

#ifdef PRODUCER
#include "xproducer.h"


static int32_t is_producer_done(void* config)
{
    return -1;
}

static void start_producer(void* config)
{
    XProducer_Start( (XProducer*)config );
}

static int32_t init_producer( void* config,void* data )
{
	int32_t rval;

    meta_data_t* ip_data = (meta_data_t*)data;

	rval = XProducer_Initialize( (XProducer*)config, ip_data->bsp_id );

    return rval;
}

static int32_t set_producer( void* config,void* data )
{

}

static void stop_producer( void* config )
{

}

pr_flow::bsp_device_t producer_table =
{
    .is_done = is_producer_done,
    .start = start_producer,
    .init = init_producer,
    .set_ptrs = set_producer,
    .stop = stop_producer,
	.debug = NULL
};


#else

pr_flow::bsp_device_t producer_table =
{
    .is_done = NULL,
    .start = NULL,
    .init = NULL,
    .set_ptrs = NULL,
    .stop = NULL,
	.debug = NULL
};

#endif



