/*
 * consumer_jump.c
 *
 *  Created on: Jun 18, 2020
 *      Author: emica
 */


#include "../../user_configs.h"
#include "../bsp_jump_table.h"
#include <stdint.h>
#include <stdlib.h>

#ifdef CONSUMER
#include "xconsumer.h"


static int32_t is_consumer_done(void* config)
{
    return -1;
}

static void start_consumer(void* config)
{
    XConsumer_Start( (XConsumer*)config );
}

static int32_t init_consumer( void* config,void* data )
{
	int32_t rval;

    meta_data_t* ip_data = (meta_data_t*)data;

	rval = XConsumer_Initialize( (XConsumer*)config, ip_data->bsp_id );

    return rval;
}

static int32_t set_consumer( void* config,void* data )
{

}

static void stop_consumer( void* config )
{

}

pr_flow::bsp_device_t consumer_table =
{
    .is_done = is_consumer_done,
    .start = start_consumer,
    .init = init_consumer,
    .set_ptrs = set_consumer,
    .stop = stop_consumer,
	.debug = NULL
};


#else

pr_flow::bsp_device_t consumer_table =
{
    .is_done = NULL,
    .start = NULL,
    .init = NULL,
    .set_ptrs = NULL,
    .stop = NULL,
	.debug = NULL
};

#endif

