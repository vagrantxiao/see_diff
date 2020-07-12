//core2
//
//ARM0: DMA_gen, Data_redir, projection_odd, projection_even,
//ARM1: rast1_even, rast1_odd, rast2_even, rast2_odd
//ARM2: zculling_top, zculling_bot, colorFB_top, colorFB_bot
//ARM3: outputBF_dull
#include "Overlays/stream.h"
#include "typedefs.h"
#include "input_data.h"

#include "Benchmarks/benchmark.h"
#include "Overlays/mmu.h"
//#define AP
//#include "ap_int.h"
// filter hidden pixels
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

void zculling_top (
#ifdef HW
		hls::stream<ap_uint<32> > & Input_1,
		hls::stream<ap_uint<32> > & Input_2,
		hls::stream<ap_uint<32> > & Output_1
#else
		pr_flow::stream & Input_1,
		pr_flow::stream & Input_2,
		pr_flow::stream & Output_1
#endif
		)
{

#ifdef HW
#pragma HLS INTERFACE ap_hs port=Input_1
#pragma HLS INTERFACE ap_hs port=Input_2
#pragma HLS INTERFACE ap_hs port=Output_1
#pragma HLS DATAFLOW
#endif


  CandidatePixel fragment;
  static bit16 counter=0;
  int i, j;
  Pixel pixels[500];
  bit16 size;
  bit32 in_tmp;
  bit32 out_tmp;
  static bit1 odd_even = 0;
  pr_flow::wide_t temp_data;


  if(odd_even == 0) size = STREAM_READ( Input_1 );
  else size = STREAM_READ( Input_2 );


  // initilize the z-buffer in rendering first triangle for an image
  static bit8 z_buffer[MAX_X/2][MAX_Y];
  if (counter == 0)
  {
    ZCULLING_INIT_ROW: for ( bit16 i = 0; i < MAX_X/2; i++)
    {
      #pragma HLS PIPELINE II=1
      ZCULLING_INIT_COL: for ( bit16 j = 0; j < MAX_Y; j++)
      {
        z_buffer[i][j] = 255;
      }
    }
  }


  // pixel counter
  bit16 pixel_cntr = 0;

  // update z-buffer and pixels
  ZCULLING: for ( bit16 n = 0; n < size; n++ )
  {
#pragma HLS PIPELINE II=1

	  if(odd_even == 0) in_tmp = STREAM_READ( Input_1 );
	  else in_tmp = STREAM_READ( Input_2 );

#ifdef AP
	fragment.x = in_tmp(7, 0);
	fragment.y = in_tmp(15, 8);
	fragment.z = in_tmp(23, 16);
	fragment.color = in_tmp(31, 24);
#else
	fragment.x = (in_tmp >> 0) & 0xFF;
	fragment.y = (in_tmp >> 8) & 0xFF;
	fragment.z = (in_tmp >> 16) & 0xFF;
	fragment.color = (in_tmp >> 24) & 0xFF;
#endif

	if( fragment.z < z_buffer[fragment.y-128][fragment.x] )
    {

      pixels[pixel_cntr].x     = fragment.x;
      pixels[pixel_cntr].y     = fragment.y;
      pixels[pixel_cntr].color = fragment.color;
      pixel_cntr++;
      z_buffer[fragment.y-128][fragment.x] = fragment.z;
    }
  }

  STREAM_WRITE( Output_1, pixel_cntr );


  for(j=0; j<pixel_cntr; j++){
#pragma HLS PIPELINE II=1
#ifdef AP
	  out_tmp(7,  0) = pixels[j].x;
      out_tmp(15, 8) = pixels[j].y;
      out_tmp(23, 16) = pixels[j].color;
	  out_tmp(31, 24) = 0;
#else
	  out_tmp = 0;
	  out_tmp |= pixels[j].x & 0xFF;
	  out_tmp |= ( pixels[j].y << 8 ) & 0x00FF00;
	  out_tmp |= ( pixels[j].color << 16 ) & 0xFF0000;
#endif

	  STREAM_WRITE( Output_1, out_tmp );

  }


  counter++;
  odd_even = ~odd_even;
  if(counter==NUM_3D_TRI){counter=0;}
  return;
}

// filter hidden pixels
void zculling_bot (
#ifdef HW
		hls::stream<ap_uint<32> > & Input_1,
		hls::stream<ap_uint<32> > & Input_2,
		hls::stream<ap_uint<32> > & Output_1
#else
		pr_flow::stream & Input_1,
		pr_flow::stream & Input_2,
		pr_flow::stream & Output_1
#endif
		)
{

#ifdef HW
#pragma HLS INTERFACE ap_hs port=Input_1
#pragma HLS INTERFACE ap_hs port=Input_2
#pragma HLS INTERFACE ap_hs port=Output_1
#pragma HLS DATAFLOW
#endif


  CandidatePixel fragment;
  static bit16 counter=0;
  int i, j;
  Pixel pixels[500];
  bit16 size;
  bit32 in_tmp;
  bit32 out_tmp;
  static bit1 odd_even = 0;
  pr_flow::wide_t temp_data;

  if(odd_even == 0) size = STREAM_READ( Input_1 );
  else size = STREAM_READ( Input_2 );


  // initilize the z-buffer in rendering first triangle for an image
  static bit8 z_buffer[MAX_X/2][MAX_Y];
  if (counter == 0)
  {
    ZCULLING_INIT_ROW: for ( bit16 i = 0; i < MAX_X/2; i++)
    {
      #pragma HLS PIPELINE II=1
      ZCULLING_INIT_COL: for ( bit16 j = 0; j < MAX_Y; j++)
      {
        z_buffer[i][j] = 255;
      }
    }
  }


  // pixel counter
  bit16 pixel_cntr = 0;

  // update z-buffer and pixels
  ZCULLING: for ( bit16 n = 0; n < size; n++ )
  {
#pragma HLS PIPELINE II=1

	  if(odd_even == 0) in_tmp = STREAM_READ( Input_1 );
	  else in_tmp = STREAM_READ( Input_2 );

#ifdef AP
	fragment.x = in_tmp(7, 0);
	fragment.y = in_tmp(15, 8);
	fragment.z = in_tmp(23, 16);
	fragment.color = in_tmp(31, 24);
#else
	fragment.x = (in_tmp >> 0) & 0xFF;
	fragment.y = (in_tmp >> 8) & 0xFF;
	fragment.z = (in_tmp >> 16) & 0xFF;
	fragment.color = (in_tmp >> 24) & 0xFF;
#endif
    if( fragment.z < z_buffer[fragment.y][fragment.x] )
    {

      pixels[pixel_cntr].x     = fragment.x;
      pixels[pixel_cntr].y     = fragment.y;
      pixels[pixel_cntr].color = fragment.color;
      pixel_cntr++;
      z_buffer[fragment.y][fragment.x] = fragment.z;
    }
  }

  STREAM_WRITE( Output_1, pixel_cntr );

  for(j=0; j<pixel_cntr; j++){
#pragma HLS PIPELINE II=1
#ifdef AP
	  out_tmp(7,  0) = pixels[j].x;
      out_tmp(15, 8) = pixels[j].y;
      out_tmp(23, 16) = pixels[j].color;
	  out_tmp(31, 24) = 0;
#else
	  out_tmp = 0;
	  out_tmp |= pixels[j].x & 0xFF;
	  out_tmp |= ( pixels[j].y << 8 ) & 0x00FF00;
	  out_tmp |= ( pixels[j].color << 16 ) & 0xFF0000;
#endif

	  STREAM_WRITE( Output_1, out_tmp );

  }


  counter++;
  odd_even = ~odd_even;
  if(counter==NUM_3D_TRI){counter=0;}
  return;
}

void render_ps( pr_flow::memory_t mem )
{

	// rx from core0
	pr_flow::stream Output_r2_odd_top( pr_flow::stream_id_t::STREAM_ID_3, pr_flow::direction_t::SW_SHARED,pr_flow::width_t::SW_WIDTH , pr_flow::axi_port_t::HP0,mem );

	// tx to core2
	pr_flow::stream Output_r2_even_top( pr_flow::stream_id_t::STREAM_ID_5, pr_flow::direction_t::SW_SHARED,pr_flow::width_t::SW_WIDTH, pr_flow::axi_port_t::HP0,mem );

    //
	pr_flow::stream Output_zcu_top( pr_flow::stream_id_t::STREAM_ID_9, pr_flow::direction_t::SW_SHARED,pr_flow::width_t::SW_WIDTH, pr_flow::axi_port_t::HP0,mem );

	synchronize();


	for (bit16 i = 0; i < NUM_3D_TRI; i++)
	{
		zculling_top(Output_r2_odd_top,Output_r2_even_top,Output_zcu_top); // stream 7 will rx from 6
	}


    Output_r2_odd_top.~stream();
    Output_r2_even_top.~stream();
    Output_zcu_top.~stream();

    synchronize();
}


void render_pl_mix( pr_flow::memory_t mem )
{

	// rx from core0
	pr_flow::stream Output_r2_odd_toprx( pr_flow::stream_id_t::STREAM_ID_0, pr_flow::direction_t::RX,pr_flow::width_t::U32_BITS, pr_flow::axi_port_t::HP0,mem );

	// tx to core2
	pr_flow::stream Output_r2_even_toprx( pr_flow::stream_id_t::STREAM_ID_2, pr_flow::direction_t::RX,pr_flow::width_t::U32_BITS,  pr_flow::axi_port_t::HP0,mem );

    //
	pr_flow::stream Output_zcu_toptx( pr_flow::stream_id_t::STREAM_ID_5, pr_flow::direction_t::TX,pr_flow::width_t::U32_BITS, pr_flow::axi_port_t::HP0,mem );

	synchronize();

	Output_r2_odd_toprx.start_stream();


	// odds receive, evens transmit
	//
	// I/O pairs: (core1 holds 6,8,10,12)
	// 6->7 core2 rx7
	// 10->11 core2 rx11
	// 12->13 core2 rx13
	// 14->15 core3 rx15
	// 16->17 core3 rx17
	// 10->11 core rx11
	// 12->13 core rx13
	for (bit16 i = 0; i < NUM_3D_TRI; i++){
		zculling_top(Output_r2_odd_toprx,Output_r2_even_toprx,Output_zcu_toptx); // stream 7 will rx from 6
	}

	//
    Output_r2_odd_toprx.~stream();
    Output_r2_even_toprx.~stream();
    Output_zcu_toptx.~stream();

	synchronize();

    //
    shutdown_ip(perf);

}

