//core1
//
//ARM0: DMA_gen, Data_redir, projection_odd, projection_even,
//ARM1: rast1_even, rast1_odd, rast2_even, rast2_odd
//ARM2: zculling_top, zculling_bot, colorFB_top, colorFB_bot
//ARM3: outputBF_dull

#include "Overlays/stream.h"
#include "Overlays/mmu.h"
#include "typedefs.h"
#include "input_data.h"
//#define HW
//#define AP

#include "Benchmarks/benchmark.h"

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

uint8_t perf = 0;

// Determine whether three vertices of a trianlgLe
// (x0,y0) (x1,y1) (x2,y2) are in clockwise order by Pineda algorithm
// if so, return a number > 0
// else if three points are in line, return a number == 0
// else in counterclockwise order, return a number < 0
int check_clockwise( Triangle_2D triangle_2d )
{
  int cw;

  cw = (triangle_2d.x2 - triangle_2d.x0) * (triangle_2d.y1 - triangle_2d.y0)
       - (triangle_2d.y2 - triangle_2d.y0) * (triangle_2d.x1 - triangle_2d.x0);

  return cw;

}

// swap (x0, y0) (x1, y1) of a Triangle_2D
void clockwise_vertices( Triangle_2D *triangle_2d )
{

  bit8 tmp_x, tmp_y;

  tmp_x = triangle_2d->x0;
  tmp_y = triangle_2d->y0;

  triangle_2d->x0 = triangle_2d->x1;
  triangle_2d->y0 = triangle_2d->y1;

  triangle_2d->x1 = tmp_x;
  triangle_2d->y1 = tmp_y;

}

// Given a pixel, determine whether it is inside the triangle
// by Pineda algorithm
// if so, return true
// else, return false
bit1 pixel_in_triangle( bit8 x, bit8 y, Triangle_2D triangle_2d )
{

  int pi0, pi1, pi2;

  pi0 = (x - triangle_2d.x0) * (triangle_2d.y1 - triangle_2d.y0) - (y - triangle_2d.y0) * (triangle_2d.x1 - triangle_2d.x0);
  pi1 = (x - triangle_2d.x1) * (triangle_2d.y2 - triangle_2d.y1) - (y - triangle_2d.y1) * (triangle_2d.x2 - triangle_2d.x1);
  pi2 = (x - triangle_2d.x2) * (triangle_2d.y0 - triangle_2d.y2) - (y - triangle_2d.y2) * (triangle_2d.x0 - triangle_2d.x2);

  return (pi0 >= 0 && pi1 >= 0 && pi2 >= 0);
}

// find the min from 3 integers
bit8 find_min( bit8 in0, bit8 in1, bit8 in2 )
{
  if (in0 < in1)
  {
    if (in0 < in2)
      return in0;
    else
      return in2;
  }
  else
  {
    if (in1 < in2)
      return in1;
    else
      return in2;
  }
}


// find the max from 3 integers
bit8 find_max( bit8 in0, bit8 in1, bit8 in2 )
{
  if (in0 > in1)
  {
    if (in0 > in2)
      return in0;
    else
      return in2;
  }
  else
  {
    if (in1 > in2)
      return in1;
    else
      return in2;
  }
}


// find pixels in the triangles from the bounding box
static void rasterization2_odd (
#ifdef HW
		hls::stream<ap_uint<32> > & Input_1,
		hls::stream<ap_uint<32> > & Output_1,
		hls::stream<ap_uint<32> > & Output_2
#else
		pr_flow::stream & Input_1,
		pr_flow::stream & Output_1,
		pr_flow::stream & Output_2
#endif
		)
{


#pragma HLS INTERFACE ap_hs port=Input_1
#pragma HLS INTERFACE ap_hs port=Output_1
#pragma HLS INTERFACE ap_hs port=Output_2
#pragma HLS DATAFLOW

	bit16 i = 0;
	bit16 i_top = 0;
	bit16 i_bot = 0;
	int y_tmp;
	int j;
	Triangle_2D triangle_2d_same;
	bit2 flag;
	bit8 max_min[5];
	bit16 max_index[1];
	bit32 out_tmp;
	static CandidatePixel fragment[500];
	pr_flow::wide_t temp_data;

	flag = STREAM_READ( Input_1 );
	triangle_2d_same.x0= STREAM_READ( Input_1 );
	triangle_2d_same.y0= STREAM_READ( Input_1 );
	triangle_2d_same.x1=STREAM_READ( Input_1 );
	triangle_2d_same.y1=STREAM_READ( Input_1 );
	triangle_2d_same.x2=STREAM_READ( Input_1 );
	triangle_2d_same.y2=STREAM_READ( Input_1 );
	triangle_2d_same.z=STREAM_READ( Input_1 );
	max_index[0]=STREAM_READ( Input_1 );
	max_min[0]=STREAM_READ( Input_1 );
	max_min[1]=STREAM_READ( Input_1 );
	max_min[2]=STREAM_READ( Input_1 );
	max_min[3]=STREAM_READ( Input_1 );
	max_min[4]=STREAM_READ( Input_1 );

	  // clockwise the vertices of input 2d triangle
	  if ( flag )
	  {
		  STREAM_WRITE( Output_1, i_top );
		  STREAM_WRITE( Output_2, i_bot );
		  return;
	  }

	bit8 color = 100;


  RAST2: for ( bit16 k = 0; k < max_index[0]; k++ )
  {

    #pragma HLS PIPELINE II=1

	  bit8 x = max_min[0] + k%max_min[4];
    bit8 y = max_min[2] + k/max_min[4];

    if( pixel_in_triangle( x, y, triangle_2d_same ) )
    {
      fragment[i].x = x;
      fragment[i].y = y;
      fragment[i].z = triangle_2d_same.z;
      fragment[i].color = color;
      i++;
      if(y>127) i_top++;
      else i_bot++;
    }
  }

	STREAM_WRITE( Output_1, i_top );
	STREAM_WRITE( Output_2, i_bot );

  for(j=0; j<i; j++){

#ifdef AP
#pragma HLS PIPELINE II=1
	  out_tmp(7, 0) = fragment[j].x;
	  out_tmp(15, 8) = fragment[j].y;
	  y_tmp = (int) out_tmp(15, 8);
	  out_tmp(23, 16) = fragment[j].z;
	  out_tmp(31, 24) = fragment[j].color;
#else
	  out_tmp = fragment[j].x &0xFF;
	  out_tmp |= (fragment[j].y << 8) & 0x0000FF00;
	  y_tmp = (fragment[j].y);
	  out_tmp |= (fragment[j].z << 16)  & 0x00FF0000;
	  out_tmp |= (fragment[j].color << 24)  & 0xFF000000;
#endif

	  if( y_tmp > 127) STREAM_WRITE( Output_1, out_tmp );
	  else STREAM_WRITE( Output_2, out_tmp );

  }

  return;
}


// find pixels in the triangles from the bounding box
static void rasterization2_even (
#ifdef HW
		hls::stream<ap_uint<32> > & Input_1,
		hls::stream<ap_uint<32> > & Output_1,
		hls::stream<ap_uint<32> > & Output_2
#else
		pr_flow::stream & Input_1,
		pr_flow::stream & Output_1,
		pr_flow::stream & Output_2
#endif
		)
{

#pragma HLS INTERFACE ap_hs port=Input_1
#pragma HLS INTERFACE ap_hs port=Output_1
#pragma HLS INTERFACE ap_hs port=Output_2
#pragma HLS DATAFLOW



	bit16 i = 0;
	bit16 i_top = 0;
	bit16 i_bot = 0;
	int y_tmp;
	int j;
	Triangle_2D triangle_2d_same;
	bit2 flag;
	bit8 max_min[5];
	bit16 max_index[1];
	bit32 out_tmp;
	static CandidatePixel fragment[500];
	pr_flow::wide_t temp_data;

	flag = STREAM_READ( Input_1 );
	triangle_2d_same.x0= STREAM_READ( Input_1 );
	triangle_2d_same.y0= STREAM_READ( Input_1 );
	triangle_2d_same.x1=STREAM_READ( Input_1 );
	triangle_2d_same.y1=STREAM_READ( Input_1 );
	triangle_2d_same.x2=STREAM_READ( Input_1 );
	triangle_2d_same.y2=STREAM_READ( Input_1 );
	triangle_2d_same.z=STREAM_READ( Input_1 );
	max_index[0]=STREAM_READ( Input_1 );
	max_min[0]=STREAM_READ( Input_1 );
	max_min[1]=STREAM_READ( Input_1 );
	max_min[2]=STREAM_READ( Input_1 );
	max_min[3]=STREAM_READ( Input_1 );
	max_min[4]=STREAM_READ( Input_1 );

	  // clockwise the vertices of input 2d triangle
	  if ( flag )
	  {
		  STREAM_WRITE( Output_1, i_top );
		  STREAM_WRITE( Output_2, i_bot );
		  return;
	  }
  bit8 color = 100;


  RAST2: for ( bit16 k = 0; k < max_index[0]; k++ )
  {
#pragma HLS PIPELINE II=1
    bit8 x = max_min[0] + k%max_min[4];
    bit8 y = max_min[2] + k/max_min[4];

    if( pixel_in_triangle( x, y, triangle_2d_same ) )
    {
      fragment[i].x = x;
      fragment[i].y = y;
      fragment[i].z = triangle_2d_same.z;
      fragment[i].color = color;
      i++;
      if(y>127) i_top++;
      else i_bot++;
    }
  }

	STREAM_WRITE( Output_1, i_top );
	STREAM_WRITE( Output_2, i_bot );

  for(j=0; j<i; j++){
#ifdef AP
#pragma HLS PIPELINE II=1
	  out_tmp(7, 0) = fragment[j].x;
	  out_tmp(15, 8) = fragment[j].y;
	  y_tmp = (int) out_tmp(15, 8);
	  out_tmp(23, 16) = fragment[j].z;
	  out_tmp(31, 24) = fragment[j].color;
#else
	  out_tmp = fragment[j].x &0xFF;
	  out_tmp |= (fragment[j].y << 8) & 0x0000FF00;
	  y_tmp = (fragment[j].y);
	  out_tmp |= (fragment[j].z << 16)  & 0x00FF0000;
	  out_tmp |= (fragment[j].color << 24)  & 0xFF000000;
#endif
	  if( y_tmp > 127) STREAM_WRITE( Output_1, out_tmp );
	  else STREAM_WRITE( Output_2, out_tmp );
  }

  return;
}


void rasterization2_m (
#ifdef HW
		hls::stream<ap_uint<32> > & Input_1,
		hls::stream<ap_uint<32> > & Output_1,
		hls::stream<ap_uint<32> > & Output_2,

		hls::stream<ap_uint<32> > & Input_2,
		hls::stream<ap_uint<32> > & Output_3,
		hls::stream<ap_uint<32> > & Output_4
#else
		pr_flow::stream & Input_1,
		pr_flow::stream & Output_1,
		pr_flow::stream & Output_2,

		pr_flow::stream & Input_2,
		pr_flow::stream & Output_3,
		pr_flow::stream & Output_4
#endif
		)
{
#pragma HLS INTERFACE ap_hs port=Input_1
#pragma HLS INTERFACE ap_hs port=Input_2
#pragma HLS INTERFACE ap_hs port=Output_1
#pragma HLS INTERFACE ap_hs port=Output_2
#pragma HLS INTERFACE ap_hs port=Output_3
#pragma HLS INTERFACE ap_hs port=Output_4

	rasterization2_odd(	Input_1,Output_1,Output_2);

	rasterization2_even( Input_2,Output_3,Output_4);

}

// filter hidden pixels
static void zculling_bot (
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
  uint16_t size;
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
	  out_tmp = pixels[j].x & 0xFF;
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

// color the frame buffer
void coloringFB_bot_m(
#ifdef HW
		hls::stream<ap_uint<32> > & Input_1,
		hls::stream<ap_uint<32> > & Output_1
#else
		pr_flow::stream & Input_1,
		pr_flow::stream & Output_1
#endif
)
{
#pragma HLS INTERFACE ap_hs port=Input_1
#pragma HLS INTERFACE ap_hs port=Output_1
  #pragma HLS INLINE off
  int i,j;
  static bit8 frame_buffer[MAX_X][MAX_Y/2];
  Pixel pixels;
  static bit16 counter=0;
  bit16 size_pixels;
  bit32 in_tmp;
  bit32 out_FB = 0;
  pr_flow::wide_t temp_data;

  size_pixels = STREAM_READ( Input_1 );

  if ( counter == 0 )
  {
    // initilize the framebuffer for a new image
    COLORING_FB_INIT_ROW: for ( bit16 i = 0; i < MAX_X; i++)
    {
      #pragma HLS PIPELINE II=1
      COLORING_FB_INIT_COL: for ( bit16 j = 0; j < MAX_Y/2; j++)
        frame_buffer[i][j] = 0;
    }
  }

  // update the framebuffer
  COLORING_FB: for ( bit16 i = 0; i < size_pixels; i++)
  {
    #pragma HLS PIPELINE II=1
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
	 frame_buffer[ pixels.x ][ pixels.y ] = pixels.color;
  }

  counter++;
  if(counter==NUM_3D_TRI){
	  for (i=0; i<MAX_X; i++){
		  for(j=0; j<MAX_Y/2; j+=4){
#ifdef AP
			  out_FB( 7,  0) = frame_buffer[i][j];
			  out_FB(15,  8) = frame_buffer[i][j+1];
			  out_FB(23, 16) = frame_buffer[i][j+2];
			  out_FB(31, 24) = frame_buffer[i][j+3];
#else
			  out_FB = 0;
			  out_FB = frame_buffer[i][j] & 0xFF;
			  out_FB |= (( frame_buffer[i][j+1] << 8)  & 0x0000FF00);
			  out_FB |= (( frame_buffer[i][j+2] << 16) & 0x00FF0000);
			  out_FB |= (( frame_buffer[i][j+3] << 24) & 0xFF000000);
#endif
			  STREAM_WRITE( Output_1, out_FB );

		  }
	  }
	  counter=0;
  }
}

void render_ps( pr_flow::memory_t mem )
{

	// rx from core0
	pr_flow::stream Output_r2_odd_bot( pr_flow::stream_id_t::STREAM_ID_4, pr_flow::direction_t::SW_SHARED,pr_flow::width_t::SW_WIDTH, pr_flow::axi_port_t::HP0,mem );

	// rx from core0
	pr_flow::stream Output_r2_even_bot( pr_flow::stream_id_t::STREAM_ID_6, pr_flow::direction_t::SW_SHARED,pr_flow::width_t::SW_WIDTH, pr_flow::axi_port_t::HP0,mem );

    // same core operator
	pr_flow::stream Output_zcu_bot( pr_flow::stream_id_t::STREAM_ID_7, pr_flow::direction_t::SW_SHARED,pr_flow::width_t::SW_WIDTH, pr_flow::axi_port_t::HP0,mem );

    // tx to core3
	pr_flow::stream Output_cfb_bot( pr_flow::stream_id_t::STREAM_ID_8, pr_flow::direction_t::SW_SHARED,pr_flow::width_t::SW_WIDTH, pr_flow::axi_port_t::HP0,mem );

	//
	synchronize();

	for (bit16 i = 0; i < NUM_3D_TRI; i++){
		zculling_bot(Output_r2_odd_bot,Output_r2_even_bot,Output_zcu_bot);
		coloringFB_bot_m(Output_zcu_bot, Output_cfb_bot);
	}

	//
	Output_r2_odd_bot.~stream();
	Output_r2_even_bot.~stream();
	Output_zcu_bot.~stream();
	Output_cfb_bot.~stream();

	synchronize();

}

void render_pl_mix( pr_flow::memory_t mem )
{
	// rx from core0
	pr_flow::stream Output_r2_odd_botrx( pr_flow::stream_id_t::STREAM_ID_1, pr_flow::direction_t::RX,pr_flow::width_t::U32_BITS, pr_flow::axi_port_t::HP0,mem );

	// rx from core0
	pr_flow::stream Output_r2_even_botrx( pr_flow::stream_id_t::STREAM_ID_3, pr_flow::direction_t::RX,pr_flow::width_t::U32_BITS, pr_flow::axi_port_t::HP0,mem );

    // stream between operators on the same core
	pr_flow::stream Output_zcu_bot( pr_flow::stream_id_t::STREAM_ID_3, pr_flow::direction_t::SW_SHARED,pr_flow::width_t::U32_BITS, pr_flow::axi_port_t::HP0,mem );

    // tx to core3
	pr_flow::stream Output_cfb_bottx( pr_flow::stream_id_t::STREAM_ID_4, pr_flow::direction_t::TX,pr_flow::width_t::U32_BITS, pr_flow::axi_port_t::HP0,mem );

	//
	synchronize();

	//
	//start_ip(perf);

	//
	for (bit16 i = 0; i < NUM_3D_TRI; i++){
		zculling_bot(Output_r2_odd_botrx,Output_r2_even_botrx,Output_zcu_bot);
		coloringFB_bot_m(Output_zcu_bot, Output_cfb_bottx);
	}

	//
    Output_r2_odd_botrx.~stream();
    Output_r2_even_botrx.~stream();
    Output_zcu_bot.~stream();
    Output_cfb_bottx.~stream();

    //
	synchronize();

    //
    shutdown_ip(perf);
}
