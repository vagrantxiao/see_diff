//
//
//ARM0: DMA_gen, Data_redir, projection_odd, projection_even,
//ARM1: rast1_even, rast1_odd, rast2_even, rast2_odd
//ARM2: zculling_top, zculling_bot, colorFB_top, colorFB_bot
//ARM3: outputBF_dull

#include "Overlays/stream.h"
#include "typedefs.h"
#include "input_data.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>

//#define AP
#include "Benchmarks/benchmark.h"
#include "Overlays/mmu.h"

#include "xil_mmu.h"
#include "xtime_l.h"
#include "xstatus.h"

#define TIMER (0xFFFEB0016) // shared memory region for creation of sw streams

uint32_t output[256*256];

int place = 0;
uint8_t perf = 0;

/// macro wrappers for hw sw
#ifdef HW
	#define STREAM hls::stream<ap_uint<32> >
	#define STREAM_READ( stream ) stream.read();
	#define STREAM_WRITE( stream,value ) stream.write(value);
#else
	#define STREAM pr_flow::stream
	#define STREAM_READ( stream )(\
	{							 \
	pr_flow::wide_t temp; 		 \
	stream.read(&temp); 		 \
	temp.lower_32;					 \
	})
	#define STREAM_WRITE( stream,value )(\
	{							 \
	pr_flow::wide_t temp; 		 \
	temp.lower_32 = value;			 \
	temp.upper_32 = 0;			     \
	stream.write(temp); 	 	 \
	})
#endif


// color the frame buffer
void coloringFB_top_m(
#ifdef HW
		hls::stream<ap_uint<32> > & Input_1,
		hls::stream<ap_uint<32> > & Input_2,
		hls::stream<ap_uint<32> > & Output_1
#else
		pr_flow::stream & Input_1,
		pr_flow::stream & Input_2
#endif
)
{
#pragma HLS INTERFACE ap_hs port=Input_1
#pragma HLS INTERFACE ap_hs port=Input_2
//#pragma HLS INTERFACE ap_hs port=Output_1
  #pragma HLS INLINE off

  int i,j;
  static uint8_t frame_buffer[MAX_X][MAX_Y/2];
  Pixel pixels;
  static int counter=0;
  static unsigned short size_pixels;
  bit32 in_tmp;
  bit32 out_FB;
  pr_flow::wide_t temp_data;

  size_pixels = STREAM_READ( Input_1 );


  if ( counter == 0 )
  {
    // initilize the framebuffer for a new image
    COLORING_FB_INIT_ROW: for ( int i = 0; i < MAX_X; i++)
    {
      #pragma HLS PIPELINE II=1
      COLORING_FB_INIT_COL: for ( int j = 0; j < MAX_Y/2; j++)
        frame_buffer[i][j] = 0;
    }
  }

  // update the framebuffer
  COLORING_FB: for ( int i = 0; i < size_pixels; i++)
  {
    #pragma HLS PIPELINE II=1
//	  printf("size pixels: %d \n",size_pixels);
	  in_tmp = STREAM_READ( Input_1 );


#ifdef AP
	 pixels.x=in_tmp(7, 0);
	 pixels.y=in_tmp(15, 8);
	 pixels.color=in_tmp(23, 16);
#else
	 pixels.x = in_tmp & 0xFF;
	 pixels.y = ( in_tmp >> 8 ) & 0xFF;
	 pixels.color = ( in_tmp >> 16 ) & 0xFF;
#endif
	 frame_buffer[ pixels.x ][ pixels.y-128 ] = pixels.color;
  }

  counter++;
  if(counter==NUM_3D_TRI){
	  for (int l=0; l<16; l++){
//	    Output_1.write(16400);
	    output[place] = 16400;
	    place++;
	  }
	  for (i=0; i<MAX_X; i++){
		  for(int k=0; k<MAX_Y/2; k+=4){
			  output[place] = STREAM_READ( Input_2 );
			  place++;
		  }
		  for(j=0; j<MAX_Y/2; j+=4){
#ifdef AP
			  out_FB( 7,  0) = frame_buffer[i][j];
			  out_FB(15,  8) = frame_buffer[i][j+1];
			  out_FB(23, 16) = frame_buffer[i][j+2];
			  out_FB(31, 24) = frame_buffer[i][j+3];
#else
			  out_FB = 0;
			  out_FB = frame_buffer[i][j] & 0xFF;
			  out_FB |= ( frame_buffer[i][j+1] << 8) & 0x0000FF00;
			  out_FB |= ( frame_buffer[i][j+2] << 16) & 0x00FF0000;
			  out_FB |= ( frame_buffer[i][j+3] << 24) & 0xFF000000;
#endif
			  output[place] = out_FB;
			  place++;
		  }
	  }
	  counter=0;
  }
}




static int CheckData()
{
	  uint8_t frame_buffer_print[256][256];

	  // read result from the 32-bit output buffer
	  for (int i = 0, j = 0, n = 0; n < 256*256/4; n ++ )
	  {
		bit32 temp = output[n+16];

		frame_buffer_print[i][j++] = temp & 0x000000FF;
		frame_buffer_print[i][j++] = ((temp & 0x0000FF00) >> 8);
		frame_buffer_print[i][j++] = ((temp & 0x00FF0000) >> 16);
		frame_buffer_print[i][j++] = ((temp & 0xFF000000) >> 24);
		if(j == 256)
		{
		  i++;
		  j = 0;
		}
	  }


	// print result
	printf("Image After Rendering: \n");
	for (int j = 256 - 1; j >= 0; j -- )
	{
	  for (int i = 0; i < 256; i ++ )
	  {
		int pix;

		  pix = frame_buffer_print[i][j];

		if (pix)
		  printf("1");
		else
		  printf("0");
	  }
	  printf("\n");
	}

	return XST_SUCCESS;
}

void render_ps( pr_flow::memory_t mem )
{
	volatile XTime* ptr = (volatile XTime*)TIMER;
	Xil_SetTlbAttributes((UINTPTR)ptr, NORM_NONCACHE);
	//dmb();
	place = 0;

	pr_flow::stream Output_zcu_top( pr_flow::stream_id_t::STREAM_ID_9, pr_flow::direction_t::SW_SHARED,pr_flow::SW_WIDTH, pr_flow::axi_port_t::HP0,mem );

	pr_flow::stream Output_cfb_bot( pr_flow::stream_id_t::STREAM_ID_8, pr_flow::direction_t::SW_SHARED,pr_flow::SW_WIDTH, pr_flow::axi_port_t::HP0,mem );

	synchronize();

	TRIANGLES: for (bit16 i = 0; i < NUM_3D_TRI; i++)
	{
		coloringFB_top_m(Output_zcu_top, Output_cfb_bot);
	}

	XTime timer_end;
	XTime timer_start;
	XTime_GetTime(&timer_end);

	timer_start = *ptr;
	double bits = place * sizeof(uint64_t) * 8; // bits
	double gigabits = bits / 1000000000;
	double seconds = ((double)(timer_end - timer_start) / (COUNTS_PER_SECOND)); // useconds
	double tput = (gigabits/seconds); // b/us ->gbps
	printf("Render PS Throughput ~ %f Gbps \n",tput);
	double time = ((double)(timer_end - timer_start) / (COUNTS_PER_SECOND / 1000000));
	printf("Send and Check Time: %.2lfus\n", time);
	CheckData();
	place=0;

	Output_zcu_top.~stream();
	Output_cfb_bot.~stream();

	synchronize();

}


// only connect streams from other cores
void render_pl_mix( pr_flow::memory_t mem )
{
	volatile XTime* ptr = (volatile XTime*)TIMER;
	Xil_SetTlbAttributes((UINTPTR)ptr, NORM_NONCACHE);
	//dmb();
	place = 0;

	pr_flow::stream Output_zcu_toprx( pr_flow::stream_id_t::STREAM_ID_5, pr_flow::direction_t::RX,pr_flow::U32_BITS, pr_flow::axi_port_t::HP0,mem );

	pr_flow::stream Output_cfb_botrx( pr_flow::stream_id_t::STREAM_ID_4, pr_flow::direction_t::RX,pr_flow::U32_BITS, pr_flow::axi_port_t::HP0,mem );

	synchronize();

	// odds receive, evens transmit
	//
	// 16->17 core3 rx17
	// 18->19
	TRIANGLES: for (uint16_t i = 0; i < NUM_3D_TRI; i++)
	{
		coloringFB_top_m(Output_zcu_toprx, Output_cfb_botrx);
	}

	XTime timer_end;
	XTime timer_start;
	XTime_GetTime(&timer_end);

	timer_start = *ptr;
	double bits = place * sizeof(uint64_t) * 8; // bits
	double gigabits = bits / 1000000000;
	double seconds = ((double)(timer_end - timer_start) / (COUNTS_PER_SECOND)); // useconds
	double tput = (gigabits/seconds); // b/us ->gbps
	printf("Render PL Throughput ~ %f Gbps \n",tput);
	double time = ((double)(timer_end - timer_start) / (COUNTS_PER_SECOND / 1000000));
	printf("Send and Check Time: %.2lfus\n", time);
	CheckData();
	place=0;

	Output_zcu_toprx.~stream();
	Output_cfb_botrx.~stream();


	synchronize();

}

