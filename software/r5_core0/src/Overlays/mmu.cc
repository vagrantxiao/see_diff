#include <stdio.h>
#include <stdint.h>
//#include "stream.h"
#include "user_configs.h"
#include "xmutex.h"
#include "xil_mmu.h"
#include "sleep.h"
#include "mmu.h"
#include "assert.h"
#include "xcirc_buff_read_many.h" // reads from ocm
#include "xcirc_buff_write_many.h" // writes to ocm


#define OCM_START_REGION (0xFFFC0000)

#define SYNCHRONIZE (0xFFFE0000) // shared memory region for synchronizing the cores
#define MMU_MEM (0xFFFE0008) // shared memory region for creation of sw streams


uint64_t tx_memory_regions[HW_STREAMS];
uint64_t rx_memory_regions[HW_STREAMS];
uint64_t sw_memory_regions[SW_STREAMS];

//
XCirc_buff_read_many axi_config_tx[NUM_TX_CONFIGS];
XCirc_buff_write_many axi_config_rx[NUM_RX_CONFIGS];


XMutex Mutex[4];	/* Mutex instance */
#define MUTEX_NUM 0


static int setup_mutex()
{
	XMutex_Config *ConfigPtr;
	XStatus Status;
	u32 TimeoutCount = 0;

	u16 MutexDeviceID = 0;
	if(XPAR_CPU_ID == 0)
	{
		MutexDeviceID = XPAR_MUTEX_0_IF_0_DEVICE_ID;
	}
	if(XPAR_CPU_ID == 1)
	{
		MutexDeviceID = XPAR_MUTEX_0_IF_1_DEVICE_ID;
	}
	if(XPAR_CPU_ID == 2)
	{
		MutexDeviceID = XPAR_MUTEX_0_IF_2_DEVICE_ID;
	}
	if(XPAR_CPU_ID == 3)
	{
		MutexDeviceID = XPAR_MUTEX_0_IF_3_DEVICE_ID;
	}


	/*
	 * Lookup configuration data in the device configuration table.
	 * Use this configuration info down below when initializing this
	 * driver instance.
	 */
	ConfigPtr = XMutex_LookupConfig(MutexDeviceID);
	if (ConfigPtr == (XMutex_Config *)NULL) {
		return XST_FAILURE;
	}

	/*
	 * Perform the rest of the initialization.
	 */
	Status = XMutex_CfgInitialize(&Mutex[XPAR_CPU_ID], ConfigPtr,
					ConfigPtr->BaseAddress);
	if (Status != XST_SUCCESS)
	{
		return XST_FAILURE;
	}
	return XST_SUCCESS;
}




// starts the streams for the shared axi version
void start_ip()
{
	// init configs and then start the streams here
	if(XPAR_CPU_ID == 0)
	{
			for(int i = 0; i < NUM_RX_CONFIGS; i++)
			{
				//
				XCirc_buff_read_many_Set_reset(&axi_config_tx[i],0);
					
				//
				XCirc_buff_read_many_Set_input_r( &axi_config_tx[i],tx_memory_regions[i] );
				
				// this effectively turns our HLS block into a while 1 loop as it will restart immediately after completing.
				XCirc_buff_read_many_EnableAutoRestart( &axi_config_tx[i] );
				
				//
				usleep(5);
				
				//
				XCirc_buff_read_many_Start( &axi_config_tx[i] );
			}
			for(int i = 0; i < NUM_TX_CONFIGS; i++)
			{
				//
				XCirc_buff_write_many_Set_reset(&axi_config_rx[i],0);
				
				// sets the pointer
				XCirc_buff_write_many_Set_output_r( &axi_config_rx[i],rx_memory_regions[i] );
				
				// this effectively turns our HLS block into a while 1 loop as it will restart immediately after completing.
				XCirc_buff_write_many_EnableAutoRestart( &axi_config_rx[i] );
				
				usleep(5);
				// starts the IP
				XCirc_buff_write_many_Start( &axi_config_rx[i] );
			}
	}

}

void shutdown_ip()
{
	if(XPAR_CPU_ID == 0)
	{
		for(int i = 0; i < NUM_RX_CONFIGS; i++)
		{
			//
			XCirc_buff_read_many_Set_reset(&axi_config_tx[i],1);
			//
			usleep(5);
			//
			XCirc_buff_read_many_DisableAutoRestart( &axi_config_tx[i] );
			
			// wait for IP to finish
			while(XCirc_buff_read_many_IsDone(&axi_config_tx[i]))
			{

			}
			printf("isDone %d \n",XCirc_buff_read_many_IsDone(&axi_config_tx[i]));
			printf("Reset tx %d \n",XCirc_buff_read_many_Get_reset(&axi_config_tx[i]));
		}
		for(int i = 0; i < NUM_TX_CONFIGS; i++)
		{
			//
			XCirc_buff_write_many_Set_reset(&axi_config_rx[i],1);
			
			//
			usleep(5);
			
			//
			XCirc_buff_write_many_DisableAutoRestart( &axi_config_rx[i] );

			//
			while(XCirc_buff_write_many_IsDone(&axi_config_rx[i]))
			{

			}

			//
			printf("isDonedone %d \n",XCirc_buff_write_many_IsDone(&axi_config_rx[i]));
			printf("Reset %d \n",XCirc_buff_write_many_Get_reset(&axi_config_rx[i]));
		}
	}
}

static void init_ip()
{
	if(XPAR_CPU_ID == 0)
	{
		for(int i = 0; i < NUM_RX_CONFIGS; i++)
		{
			// creates object to handle communication with IP
			XCirc_buff_read_many_Initialize( &axi_config_tx[i], i );

			//
			XCirc_buff_read_many_EnableAutoRestart( &axi_config_tx[i] );

			//
			XCirc_buff_read_many_Set_input_r( &axi_config_tx[i],tx_memory_regions[i] );
		}
		for(int i = 0; i < NUM_TX_CONFIGS; i++)
		{
			// creates object to handle communication with IP
			XCirc_buff_write_many_Initialize( &axi_config_rx[i], i );

			//
			XCirc_buff_write_many_EnableAutoRestart( &axi_config_rx[i] );

			//
			XCirc_buff_write_many_Set_output_r(  &axi_config_rx[i],rx_memory_regions[i] );
		}
	}

}


//
// initializes the memory parameters for HW and SW streams
// we configure our IP memory
// since we have many streams per IP we need partition the TC and RX regions
// our IP will move from stride to stride in the TX or RX memory region
// this is like a global init to setup our IP memory regions 
//
void init()
{
	uint32_t start = OCM_START_REGION;
	uint32_t end_region = OCM_START_REGION + (HW_STREAMS*2) * ( pr_flow::RAW_BUFFER_SIZE * sizeof(uint64_t) );
	int idx = 0;

	// here we configure out IP memory regions
	// if our buffer size is 10 and we have 6 streams 
	// we partition 60 bytes txstream0 gets 0-9 txstream1 gets 10-19 etc
	// txstream0 and rxstream0 will associate with each other 
	for(int i = 0; i < HW_STREAMS; i++)
	{
		tx_memory_regions[i] = start + ( i * ( pr_flow::RAW_BUFFER_SIZE * sizeof(uint64_t) ) );
	}
	// here we configure out IP memory regions
	// if our buffer size is 10 and we have 6 streams 
	// we partition 60 bytes rxstream0 gets 0-9 rxstream1 gets 10-19 etc
	// txstream0 and rxstream0 will associate with each other 
	for(int i = HW_STREAMS; i < HW_STREAMS*2; i++)
	{
		rx_memory_regions[idx] = start + ( i * ( pr_flow::RAW_BUFFER_SIZE * sizeof(uint64_t) ) );
		idx++;
	}
	// create our memoryregions for software defined streams
	for(int i = 0; i < SW_STREAMS; i++)
	{
		sw_memory_regions[i] = end_region + ( i * ( pr_flow::RAW_BUFFER_SIZE * sizeof(uint64_t) ) );
	}

	volatile int* ptr = (volatile int*)SYNCHRONIZE;
	Xil_SetTlbAttributes((UINTPTR)ptr, NORM_NONCACHE);
	dmb();

	ptr = (volatile int*)MMU_MEM;
	Xil_SetTlbAttributes((UINTPTR)ptr, NORM_NONCACHE);
	dmb();

	// init configs and then start the streams here
	init_ip();

	setup_mutex();

}

//
volatile uint64_t get_memory( pr_flow::stream_id_t stream_id, pr_flow::direction_t direction  )
{
	if(direction == pr_flow::SW_SHARED){
		return sw_memory_regions[stream_id];
	}
	else if(direction == pr_flow::TX){
		return tx_memory_regions[stream_id];
	}
	else if(direction == pr_flow::RX){
		return rx_memory_regions[stream_id];
	}
	else{
		assert(0);
	}
}


void synchronize()
{
	volatile int* ptr = (volatile int*)SYNCHRONIZE;
	Xil_SetTlbAttributes((UINTPTR)ptr, NORM_NONCACHE);
	dmb();

	while(1)
	{
		XMutex_Lock(&Mutex[XPAR_CPU_ID], MUTEX_NUM);	/* Acquire lock */

		// default value of OCM is set to 0xdeadbeef so first processor to grab lock can clear
		if(*ptr == 0xdeadbeef || ((*ptr % ACTIVE_CORES) == 0) )
		{
			*ptr = 1;
			XMutex_Unlock(&Mutex[XPAR_CPU_ID], MUTEX_NUM);	/* Release lock */
			break;
		}
		else
		{
			*ptr = *ptr + 1;
			XMutex_Unlock(&Mutex[XPAR_CPU_ID], MUTEX_NUM);	/* Release lock */
			break;
		}
	}
	while(*ptr != ACTIVE_CORES)
	{
	//	usleep(1);
	}
}

void get_lock()
{
	XMutex_Lock(&Mutex[XPAR_CPU_ID], MUTEX_NUM);	/* Acquire lock */
}

void release_lock()
{
	XMutex_Unlock(&Mutex[XPAR_CPU_ID], MUTEX_NUM);	/* Release lock */
}
