// ARM0: Data_In, Data_redir, projection_odd, projection_even,
// ARM1: rast1_even, rast1_odd, rast2_even, rast2_odd
// ARM2: zculling_top, zculling_bot, colorFB_top, colorFB_bot
// ARM3: outputBF_dull
#include "Overlays/stream.h"
#include "typedefs.h"
#include "input_data.h"

#include "Benchmarks/benchmark.h"
#include "Overlays/mmu.h"

#include "xil_mmu.h"
#include "xtime_l.h"

#define TIMER (0xFFFE00016) // shared memory region for creation of sw streams
//#define AP

// Determine whether three vertices of a trianlgLe
// (x0,y0) (x1,y1) (x2,y2) are in clockwise order by Pineda algorithm
// if so, return a number > 0
// else if three points are in line, return a number == 0
// else in counterclockwise order, return a number < 0
static int check_clockwise( Triangle_2D triangle_2d )
{
  int cw;

  cw = (triangle_2d.x2 - triangle_2d.x0) * (triangle_2d.y1 - triangle_2d.y0)
       - (triangle_2d.y2 - triangle_2d.y0) * (triangle_2d.x1 - triangle_2d.x0);

  return cw;

}

// swap (x0, y0) (x1, y1) of a Triangle_2D
static void clockwise_vertices( Triangle_2D *triangle_2d )
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
static bit8 find_min( bit8 in0, bit8 in1, bit8 in2 )
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
static bit8 find_max( bit8 in0, bit8 in1, bit8 in2 )
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

	flag = (bit2) Input_1.read();
	triangle_2d_same.x0=Input_1.read();
	triangle_2d_same.y0=Input_1.read();
	triangle_2d_same.x1=Input_1.read();
	triangle_2d_same.y1=Input_1.read();
	triangle_2d_same.x2=Input_1.read();
	triangle_2d_same.y2=Input_1.read();
	triangle_2d_same.z=Input_1.read();
	max_index[0]=Input_1.read();
	max_min[0]=Input_1.read();
	max_min[1]=Input_1.read();
	max_min[2]=Input_1.read();
	max_min[3]=Input_1.read();
	max_min[4]=Input_1.read();

  // clockwise the vertices of input 2d triangle
  if ( flag )
  {
	  Output_1.write(i_top);
	  Output_2.write(i_bot);
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

  Output_1.write(i_top);
  Output_2.write(i_bot);
  for(j=0; j<i; j++){

#ifdef AP
#pragma HLS PIPELINE II=1
	  out_tmp(7, 0) = fragment[j].x;
	  out_tmp(15, 8) = fragment[j].y;
	  y_tmp = (int) out_tmp(15, 8);
	  out_tmp(23, 16) = fragment[j].z;
	  out_tmp(31, 24) = fragment[j].color;
#else
	  out_tmp = fragment[j].x;
	  out_tmp |= (fragment[j].y << 8);
//	  y_tmp = (out_tmp >> 8) & 0xFF; // TODO check
	  y_tmp = (fragment[j].y);
	  out_tmp |= (fragment[j].z << 16);
	  out_tmp |= (fragment[j].color << 24);
#endif
	  if( y_tmp > 127) Output_1.write(out_tmp);
	  else Output_2.write(out_tmp);
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

	flag = (bit2) Input_1.read();
	triangle_2d_same.x0=Input_1.read();
	triangle_2d_same.y0=Input_1.read();
	triangle_2d_same.x1=Input_1.read();
	triangle_2d_same.y1=Input_1.read();
	triangle_2d_same.x2=Input_1.read();
	triangle_2d_same.y2=Input_1.read();
	triangle_2d_same.z=Input_1.read();
	max_index[0]=Input_1.read();
	max_min[0]=Input_1.read();
	max_min[1]=Input_1.read();
	max_min[2]=Input_1.read();
	max_min[3]=Input_1.read();
	max_min[4]=Input_1.read();

  // clockwise the vertices of input 2d triangle
  if ( flag )
  {
	  Output_1.write(i_top);
	  Output_2.write(i_bot);
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

  Output_1.write(i_top);
  Output_2.write(i_bot);
  for(j=0; j<i; j++){
#ifdef AP
#pragma HLS PIPELINE II=1
	  out_tmp(7, 0) = fragment[j].x;
	  out_tmp(15, 8) = fragment[j].y;
	  y_tmp = (int) out_tmp(15, 8);
	  out_tmp(23, 16) = fragment[j].z;
	  out_tmp(31, 24) = fragment[j].color;
#else
	  out_tmp = fragment[j].x;
	  out_tmp |= (fragment[j].y << 8);
//	  y_tmp = (out_tmp >> 8) & 0xFF; // TODO check
	  y_tmp = (fragment[j].y);
	  out_tmp |= (fragment[j].z << 16);
	  out_tmp |= (fragment[j].color << 24);
#endif
	  if(y_tmp > 127) Output_1.write(out_tmp);
	  else Output_2.write(out_tmp);
  }

  return;
}


static void rasterization2_m (
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
//#pragma HLS INTERFACE ap_hs port=Input_1
//#pragma HLS INTERFACE ap_hs port=Input_2
//#pragma HLS INTERFACE ap_hs port=Output_1
//#pragma HLS INTERFACE ap_hs port=Output_2
//#pragma HLS INTERFACE ap_hs port=Output_3
//#pragma HLS INTERFACE ap_hs port=Output_4

	rasterization2_odd(	Input_1,Output_1,Output_2);

	rasterization2_even( Input_2,Output_3,Output_4);

}


void data_in(

		pr_flow::stream & Output_1
)
{

	static int i = 0;
	// one stage of the triangle
	// 3 writes
//	for(int i = 0; i < num_3d_triangles; i++)
//	{

		uint32_t packed = 0;

		packed = triangle_3ds[i].x0;
		packed |= ( triangle_3ds[i].y0 << 8 );
		packed |= ( triangle_3ds[i].z0 << 16 );
		packed |= ( triangle_3ds[i].x1 << 24 );
		Output_1.write(packed);

		packed=0;
		packed =  triangle_3ds[i].y1;
		packed |= ( triangle_3ds[i].z1 << 8 );
		packed |= ( triangle_3ds[i].x2 << 16 );
		packed |= ( triangle_3ds[i].y2 << 24 );
		Output_1.write(packed);

		packed = 0;
		packed = triangle_3ds[i].z2;
		Output_1.write(packed);
		i++;
		//	}
}

// project a 3D triangle to a 2D triangle
static void projection_odd_m (
		bit32 input_lo,
		bit32 input_mi,
		bit32 input_hi,
		Triangle_2D *triangle_2d
		)
{
  #pragma HLS INLINE off
  Triangle_3D triangle_3d;
  // Setting camera to (0,0,-1), the canvas at z=0 plane
  // The 3D model lies in z>0 space
  // The coordinate on canvas is proportional to the corresponding coordinate
  // on space

    bit2 angle = 0;
#ifdef AP
    triangle_3d.x0 = input_lo( 7,  0);
    triangle_3d.y0 = input_lo(15,  8);
    triangle_3d.z0 = input_lo(23, 16);
    triangle_3d.x1 = input_lo(31, 24);
    triangle_3d.y1 = input_mi( 7,  0);
    triangle_3d.z1 = input_mi(15,  8);
    triangle_3d.x2 = input_mi(23, 16);
    triangle_3d.y2 = input_mi(31, 24);
    triangle_3d.z2 = input_hi( 7,  0);
#else
    triangle_3d.x0 = ( input_lo >> 0 ) & 0xFF;
    triangle_3d.y0 = ( input_lo >> 8 ) & 0xFF;
    triangle_3d.z0 = ( input_lo >> 16 ) & 0xFF;
    triangle_3d.x1 = ( input_lo >> 24 ) & 0xFF;
    triangle_3d.y1 = ( input_mi >> 0 ) & 0xFF;
    triangle_3d.z1 = ( input_mi >> 8 ) & 0xFF;
    triangle_3d.x2 = ( input_mi >> 16 ) & 0xFF;
    triangle_3d.y2 = ( input_mi >> 24 ) & 0xFF;
    triangle_3d.z2 = ( input_hi >> 0 ) & 0xFF;
#endif

  if(angle == 0)
  {
    triangle_2d->x0 = triangle_3d.x0;
    triangle_2d->y0 = triangle_3d.y0;
    triangle_2d->x1 = triangle_3d.x1;
    triangle_2d->y1 = triangle_3d.y1;
    triangle_2d->x2 = triangle_3d.x2;
    triangle_2d->y2 = triangle_3d.y2;
    triangle_2d->z  = triangle_3d.z0 / 3 + triangle_3d.z1 / 3 + triangle_3d.z2 / 3;
  }

  else if(angle == 1)
  {
    triangle_2d->x0 = triangle_3d.x0;
    triangle_2d->y0 = triangle_3d.z0;
    triangle_2d->x1 = triangle_3d.x1;
    triangle_2d->y1 = triangle_3d.z1;
    triangle_2d->x2 = triangle_3d.x2;
    triangle_2d->y2 = triangle_3d.z2;
    triangle_2d->z  = triangle_3d.y0 / 3 + triangle_3d.y1 / 3 + triangle_3d.y2 / 3;
  }

  else if(angle == 2)
  {
    triangle_2d->x0 = triangle_3d.z0;
    triangle_2d->y0 = triangle_3d.y0;
    triangle_2d->x1 = triangle_3d.z1;
    triangle_2d->y1 = triangle_3d.y1;
    triangle_2d->x2 = triangle_3d.z2;
    triangle_2d->y2 = triangle_3d.y2;
    triangle_2d->z  = triangle_3d.x0 / 3 + triangle_3d.x1 / 3 + triangle_3d.x2 / 3;
  }

}

// project a 3D triangle to a 2D triangle
static void projection_even_m (
		bit32 input_lo,
		bit32 input_mi,
		bit32 input_hi,
		Triangle_2D *triangle_2d
		)
{
  #pragma HLS INLINE off
  Triangle_3D triangle_3d;
  // Setting camera to (0,0,-1), the canvas at z=0 plane
  // The 3D model lies in z>0 space
  // The coordinate on canvas is proportional to the corresponding coordinate
  // on space


    bit2 angle = 0;
#ifdef AP
    triangle_3d.x0 = input_lo( 7,  0);
    triangle_3d.y0 = input_lo(15,  8);
    triangle_3d.z0 = input_lo(23, 16);
    triangle_3d.x1 = input_lo(31, 24);
    triangle_3d.y1 = input_mi( 7,  0);
    triangle_3d.z1 = input_mi(15,  8);
    triangle_3d.x2 = input_mi(23, 16);
    triangle_3d.y2 = input_mi(31, 24);
    triangle_3d.z2 = input_hi( 7,  0);
#else
    triangle_3d.x0 = ( input_lo >> 0 ) & 0xFF;
    triangle_3d.y0 = ( input_lo >> 8 ) & 0xFF;
    triangle_3d.z0 = ( input_lo >> 16 ) & 0xFF;
    triangle_3d.x1 = ( input_lo >> 24 ) & 0xFF;
    triangle_3d.y1 = ( input_mi >> 0 ) & 0xFF;
    triangle_3d.z1 = ( input_mi >> 8 ) & 0xFF;
    triangle_3d.x2 = ( input_mi >> 16 ) & 0xFF;
    triangle_3d.y2 = ( input_mi >> 24 ) & 0xFF;
    triangle_3d.z2 = ( input_hi >> 0 ) & 0xFF;
#endif

  if(angle == 0)
  {
    triangle_2d->x0 = triangle_3d.x0;
    triangle_2d->y0 = triangle_3d.y0;
    triangle_2d->x1 = triangle_3d.x1;
    triangle_2d->y1 = triangle_3d.y1;
    triangle_2d->x2 = triangle_3d.x2;
    triangle_2d->y2 = triangle_3d.y2;
    triangle_2d->z  = triangle_3d.z0 / 3 + triangle_3d.z1 / 3 + triangle_3d.z2 / 3;
  }

  else if(angle == 1)
  {
    triangle_2d->x0 = triangle_3d.x0;
    triangle_2d->y0 = triangle_3d.z0;
    triangle_2d->x1 = triangle_3d.x1;
    triangle_2d->y1 = triangle_3d.z1;
    triangle_2d->x2 = triangle_3d.x2;
    triangle_2d->y2 = triangle_3d.z2;
    triangle_2d->z  = triangle_3d.y0 / 3 + triangle_3d.y1 / 3 + triangle_3d.y2 / 3;
  }

  else if(angle == 2)
  {
    triangle_2d->x0 = triangle_3d.z0;
    triangle_2d->y0 = triangle_3d.y0;
    triangle_2d->x1 = triangle_3d.z1;
    triangle_2d->y1 = triangle_3d.y1;
    triangle_2d->x2 = triangle_3d.z2;
    triangle_2d->y2 = triangle_3d.y2;
    triangle_2d->z  = triangle_3d.x0 / 3 + triangle_3d.x1 / 3 + triangle_3d.x2 / 3;
  }
}

// calculate bounding box for a 2D triangle
static void rasterization1_odd_m (
		Triangle_2D triangle_2d,
#ifdef HW
		hls::stream<ap_uint<32> > & Output_1
#else
		pr_flow::stream & Output_1
#endif
)
{
	Triangle_2D triangle_2d_same = {0,0,0,0,0};
	static bit8 max_min[5]={0, 0, 0, 0, 0};
	static bit16 max_index[1]={0};


  #pragma HLS INLINE off
  // clockwise the vertices of input 2d triangle
  if ( check_clockwise( triangle_2d ) == 0 ){
	Output_1.write(1);
	Output_1.write(triangle_2d_same.x0);
	Output_1.write(triangle_2d_same.y0);
	Output_1.write(triangle_2d_same.x1);
	Output_1.write(triangle_2d_same.y1);
	Output_1.write(triangle_2d_same.x2);
	Output_1.write(triangle_2d_same.y2);
	Output_1.write(triangle_2d_same.z);
	Output_1.write(max_index[0]);
	Output_1.write(max_min[0]);
	Output_1.write(max_min[1]);
	Output_1.write(max_min[2]);
	Output_1.write(max_min[3]);
	Output_1.write(max_min[4]);
    return;
  }
  if ( check_clockwise( triangle_2d ) < 0 )
    clockwise_vertices( &triangle_2d );




  // copy the same 2D triangle
  triangle_2d_same.x0 = triangle_2d.x0;
  triangle_2d_same.y0 = triangle_2d.y0;
  triangle_2d_same.x1 = triangle_2d.x1;
  triangle_2d_same.y1 = triangle_2d.y1;
  triangle_2d_same.x2 = triangle_2d.x2;
  triangle_2d_same.y2 = triangle_2d.y2;
  triangle_2d_same.z  = triangle_2d.z ;

  // find the rectangle bounds of 2D triangles
  max_min[0] = find_min( triangle_2d.x0, triangle_2d.x1, triangle_2d.x2 );
  max_min[1] = find_max( triangle_2d.x0, triangle_2d.x1, triangle_2d.x2 );
  max_min[2] = find_min( triangle_2d.y0, triangle_2d.y1, triangle_2d.y2 );
  max_min[3] = find_max( triangle_2d.y0, triangle_2d.y1, triangle_2d.y2 );
  max_min[4] = max_min[1] - max_min[0];

  // calculate index for searching pixels
  max_index[0] = (max_min[1] - max_min[0]) * (max_min[3] - max_min[2]);

  Output_1.write(0);
  Output_1.write(triangle_2d_same.x0);
  Output_1.write(triangle_2d_same.y0);
  Output_1.write(triangle_2d_same.x1);
  Output_1.write(triangle_2d_same.y1);
  Output_1.write(triangle_2d_same.x2);
  Output_1.write(triangle_2d_same.y2);
  Output_1.write(triangle_2d_same.z);
  Output_1.write(max_index[0]);
  Output_1.write(max_min[0]);
  Output_1.write(max_min[1]);
  Output_1.write(max_min[2]);
  Output_1.write(max_min[3]);
  Output_1.write(max_min[4]);
  return;
}


// calculate bounding box for a 2D triangle
static void rasterization1_even_m (
		Triangle_2D triangle_2d,
#ifdef HW
		hls::stream<ap_uint<32> > & Output_1
#else
		pr_flow::stream & Output_1
#endif
)
{
	Triangle_2D triangle_2d_same = {0,0,0,0,0};
	static bit8 max_min[5]={0, 0, 0, 0, 0};
	static bit16 max_index[1]={0};


  #pragma HLS INLINE off
  // clockwise the vertices of input 2d triangle
  if ( check_clockwise( triangle_2d ) == 0 ){
	Output_1.write(1);
	Output_1.write(triangle_2d_same.x0);
	Output_1.write(triangle_2d_same.y0);
	Output_1.write(triangle_2d_same.x1);
	Output_1.write(triangle_2d_same.y1);
	Output_1.write(triangle_2d_same.x2);
	Output_1.write(triangle_2d_same.y2);
	Output_1.write(triangle_2d_same.z);
	Output_1.write(max_index[0]);
	Output_1.write(max_min[0]);
	Output_1.write(max_min[1]);
	Output_1.write(max_min[2]);
	Output_1.write(max_min[3]);
	Output_1.write(max_min[4]);
    return;
  }
  if ( check_clockwise( triangle_2d ) < 0 )
    clockwise_vertices( &triangle_2d );




  // copy the same 2D triangle
  triangle_2d_same.x0 = triangle_2d.x0;
  triangle_2d_same.y0 = triangle_2d.y0;
  triangle_2d_same.x1 = triangle_2d.x1;
  triangle_2d_same.y1 = triangle_2d.y1;
  triangle_2d_same.x2 = triangle_2d.x2;
  triangle_2d_same.y2 = triangle_2d.y2;
  triangle_2d_same.z  = triangle_2d.z ;

  // find the rectangle bounds of 2D triangles
  max_min[0] = find_min( triangle_2d.x0, triangle_2d.x1, triangle_2d.x2 );
  max_min[1] = find_max( triangle_2d.x0, triangle_2d.x1, triangle_2d.x2 );
  max_min[2] = find_min( triangle_2d.y0, triangle_2d.y1, triangle_2d.y2 );
  max_min[3] = find_max( triangle_2d.y0, triangle_2d.y1, triangle_2d.y2 );
  max_min[4] = max_min[1] - max_min[0];

  // calculate index for searching pixels
  max_index[0] = (max_min[1] - max_min[0]) * (max_min[3] - max_min[2]);

  Output_1.write(0);
  Output_1.write(triangle_2d_same.x0);
  Output_1.write(triangle_2d_same.y0);
  Output_1.write(triangle_2d_same.x1);
  Output_1.write(triangle_2d_same.y1);
  Output_1.write(triangle_2d_same.x2);
  Output_1.write(triangle_2d_same.y2);
  Output_1.write(triangle_2d_same.z);
  Output_1.write(max_index[0]);
  Output_1.write(max_min[0]);
  Output_1.write(max_min[1]);
  Output_1.write(max_min[2]);
  Output_1.write(max_min[3]);
  Output_1.write(max_min[4]);
  return;
}


// project a 3D triangle to a 2D triangle
void data_redir_m (
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

  bit32 input_lo;
  bit32 input_mi;
  bit32 input_hi;

  Triangle_2D triangle_2ds_1;
  Triangle_2D triangle_2ds_2;

  //
  input_lo = Input_1.read();
  input_mi = Input_1.read();
  input_hi = Input_1.read();

  //
  projection_odd_m (input_lo,input_mi,input_hi,&triangle_2ds_1);

  //
  rasterization1_odd_m(triangle_2ds_1,Output_1);

  //
  input_lo = Input_1.read();
  input_mi = Input_1.read();
  input_hi = Input_1.read();

  //
  projection_even_m (input_lo,input_mi,input_hi,&triangle_2ds_2);

  //
  rasterization1_even_m (triangle_2ds_2,Output_2);
}

void render_ps()
{
//	XTime timer_start;
//	volatile XTime* ptr = (volatile XTime*)TIMER;
//
//	XTime_StartTimer();
//
//	Xil_SetTlbAttributes((UINTPTR)ptr, NORM_NONCACHE);
//	dmb();


	 // both tx and rx when doing ps to ps
	 pr_flow::stream Input_1( pr_flow::stream_id_t::STREAM_ID_SW0, pr_flow::direction_t::SW_SHARED, pr_flow::axi_port_t::HP0 );

	 // both tx and rx when doing ps to ps
	 pr_flow::stream Output_redir_odd( pr_flow::stream_id_t::STREAM_ID_SW1, pr_flow::direction_t::SW_SHARED, pr_flow::axi_port_t::HP0 );

	 // both tx and rx when doing ps to ps
	 pr_flow::stream Output_redir_even( pr_flow::stream_id_t::STREAM_ID_SW2, pr_flow::direction_t::SW_SHARED, pr_flow::axi_port_t::HP0 );


	 // rx from core0
	 pr_flow::stream Output_r2_odd_top( pr_flow::stream_id_t::STREAM_ID_SW3, pr_flow::direction_t::SW_SHARED, pr_flow::axi_port_t::HP0 );

	 // rx from core0
	 pr_flow::stream Output_r2_odd_bot( pr_flow::stream_id_t::STREAM_ID_SW4, pr_flow::direction_t::SW_SHARED, pr_flow::axi_port_t::HP0 );

	 // tx to core2
	 pr_flow::stream Output_r2_even_top( pr_flow::stream_id_t::STREAM_ID_SW5, pr_flow::direction_t::SW_SHARED, pr_flow::axi_port_t::HP0 );

	 pr_flow::stream Output_r2_even_bot( pr_flow::stream_id_t::STREAM_ID_SW6, pr_flow::direction_t::SW_SHARED, pr_flow::axi_port_t::HP0 );

	synchronize();

//	XTime_GetTime(&timer_start);
//	*ptr = timer_start;

	for (bit16 i = 0; i < NUM_3D_TRI/2; i++)
	{
		data_in(Input_1); // stream 0 txs
		data_in(Input_1); // stream 0 txs
		data_redir_m(Input_1,Output_redir_odd,Output_redir_even); // 1 rx 2 4 tx
		rasterization2_m(Output_redir_odd,Output_r2_odd_top,Output_r2_odd_bot,Output_redir_even,Output_r2_even_top,Output_r2_even_bot);
	}

	// clean up all streams
	Input_1.~stream();
	Output_redir_odd.~stream();
	Output_redir_even.~stream();
	Output_r2_odd_top.~stream();
	Output_r2_odd_bot.~stream();
	Output_r2_even_top.~stream();
	Output_r2_even_bot.~stream();

	synchronize();

}

//void render_pl()
//{
//
//	 // both tx and rx when doing ps to ps
//	pr_flow::stream Input_1tx( pr_flow::stream_id_t::STREAM_ID_HW_TX0, pr_flow::direction_t::TX, pr_flow::memory_t::OCM, pr_flow::axi_port_t::HP0 );
//
//	 // both tx and rx when doing ps to ps
//	pr_flow::stream Input_1rx( pr_flow::stream_id_t::STREAM_ID_HW_RX0, pr_flow::direction_t::RX, pr_flow::memory_t::OCM, pr_flow::axi_port_t::HP0 );
//
//	 // both tx and rx when doing ps to ps
//	pr_flow::stream Output_redir_oddtx( pr_flow::stream_id_t::STREAM_ID_HW_TX1, pr_flow::direction_t::TX, pr_flow::memory_t::OCM, pr_flow::axi_port_t::HP0 );
//
//	 // both tx and rx when doing ps to ps
//	pr_flow::stream Output_redir_oddrx( pr_flow::stream_id_t::STREAM_ID_HW_RX1, pr_flow::direction_t::RX, pr_flow::memory_t::OCM, pr_flow::axi_port_t::HP0 );
//
//	pr_flow::stream Output_redir_eventx( pr_flow::stream_id_t::STREAM_ID_HW_TX2, pr_flow::direction_t::TX, pr_flow::memory_t::OCM, pr_flow::axi_port_t::HP0 );
//
//	pr_flow::stream Output_redir_evenrx( pr_flow::stream_id_t::STREAM_ID_HW_RX2, pr_flow::direction_t::RX, pr_flow::memory_t::OCM, pr_flow::axi_port_t::HP0 );
//
//	 // rx from core0
//	pr_flow::stream Output_r2_odd_toptx( pr_flow::stream_id_t::STREAM_ID_HW_TX3, pr_flow::direction_t::TX, pr_flow::memory_t::OCM, pr_flow::axi_port_t::HP0 );
//
//	 // rx from core0
//	pr_flow::stream Output_r2_odd_bottx( pr_flow::stream_id_t::STREAM_ID_HW_TX4, pr_flow::direction_t::TX, pr_flow::memory_t::OCM, pr_flow::axi_port_t::HP0 );
//
//	 // tx to core2
//	pr_flow::stream Output_r2_even_toptx( pr_flow::stream_id_t::STREAM_ID_HW_TX5, pr_flow::direction_t::TX, pr_flow::memory_t::OCM, pr_flow::axi_port_t::HP0 );
//
//	pr_flow::stream Output_r2_even_bottx( pr_flow::stream_id_t::STREAM_ID_HW_TX6, pr_flow::direction_t::TX, pr_flow::memory_t::OCM, pr_flow::axi_port_t::HP0 );
//
//	 get_lock();
//
//
//	 Input_1tx.stream_init();
//	 Input_1rx.stream_init();
//
////	 Output_redir_oddtx.stream_init();
////	 Output_redir_oddrx.stream_init();
////
////	 Output_redir_eventx.stream_init();
////	 Output_redir_evenrx.stream_init();
////
////	 Output_r2_odd_toptx.stream_init();
////	 Output_r2_odd_bottx.stream_init();
////
////	 Output_r2_even_toptx.stream_init();
////	 Output_r2_even_bottx.stream_init();
//
//	// starts the PL IP associated
////	 Input_1tx.start_stream();
////	 Input_1rx.start_stream();
//
////	 Output_redir_oddtx.start_stream();
////	 Output_redir_oddrx.start_stream();
////
////	 Output_redir_eventx.start_stream();
////	 Output_redir_evenrx.start_stream();
////
////	 Output_r2_odd_toptx.start_stream();
////	 Output_r2_odd_bottx.start_stream();
////
////	 Output_r2_even_toptx.start_stream();
////	 Output_r2_even_bottx.start_stream();
//
//	 release_lock();
//
//	synchronize();
//
//	 Input_1tx.start_stream();
//	 Input_1rx.start_stream();
//
//
//	// odds receive, evens transmit
//	//
//	// I/O pairs:
//	// 0->1
//	// 2->3
//	// 4->5
//	// 6->7 core1 rx7
//	// 8->9 core1 rx9
//	// 10->11 core rx11
//	// 12->13 core rx13
//	//
//	for (bit16 i = 0; i < NUM_3D_TRI/2; i++)
//	{
//		data_in(Input_1tx); // stream 0 txs
//		data_in(Input_1tx); // stream 0 txs
//		data_redir_m(Input_1rx,Output_redir_oddtx,Output_redir_eventx); // 1 rx 2 4 tx
//		rasterization2_m(Output_redir_oddrx,Output_r2_odd_toptx,Output_r2_odd_bottx,Output_redir_evenrx,Output_r2_even_toptx,Output_r2_even_bottx);
//	}
//
//
//	// clean
//	 Input_1tx.~stream();
//	 Input_1rx.~stream();
//
//	 Output_redir_oddtx.~stream();
//	 Output_redir_oddrx.~stream();
//
//	 Output_redir_eventx.~stream();
//	 Output_redir_evenrx.~stream();
//
//	 Output_r2_odd_toptx.~stream();
//	 Output_r2_odd_bottx.~stream();
//
//	 Output_r2_even_toptx.~stream();
//	 Output_r2_even_bottx.~stream();
//
//
//	synchronize();
//}

void render_pl_mix()
{

//	XTime timer_start;
//	volatile XTime* ptr = (volatile XTime*)TIMER;
//
//	XTime_StartTimer();
//
//	Xil_SetTlbAttributes((UINTPTR)ptr, NORM_NONCACHE);
//	dmb();

	 // both tx and rx when doing ps to ps
	pr_flow::stream Input_1( pr_flow::stream_id_t::STREAM_ID_SW0, pr_flow::direction_t::SW_SHARED, pr_flow::axi_port_t::HP0 );

	 // both tx and rx when doing ps to ps
	pr_flow::stream Output_redir_odd( pr_flow::stream_id_t::STREAM_ID_SW1, pr_flow::direction_t::SW_SHARED, pr_flow::axi_port_t::HP0 );

	pr_flow::stream Output_redir_even( pr_flow::stream_id_t::STREAM_ID_SW2, pr_flow::direction_t::SW_SHARED, pr_flow::axi_port_t::HP0 );

	 // rx from core0
	pr_flow::stream Output_r2_odd_toptx( pr_flow::stream_id_t::STREAM_ID_HW_TX0, pr_flow::direction_t::TX, pr_flow::axi_port_t::HP0 );

	 // rx from core0
	pr_flow::stream Output_r2_odd_bottx( pr_flow::stream_id_t::STREAM_ID_HW_TX1, pr_flow::direction_t::TX, pr_flow::axi_port_t::HP0 );

	 // tx to core2
	pr_flow::stream Output_r2_even_toptx( pr_flow::stream_id_t::STREAM_ID_HW_TX2, pr_flow::direction_t::TX, pr_flow::axi_port_t::HP0 );

	pr_flow::stream Output_r2_even_bottx( pr_flow::stream_id_t::STREAM_ID_HW_TX3, pr_flow::direction_t::TX, pr_flow::axi_port_t::HP0 );


	 synchronize();

	 start_ip();

//	 XTime_GetTime(&timer_start);
//	 *ptr = timer_start;


	// odds receive, evens transmit
	//
	// I/O pairs:
	// 0->1
	// 2->3
	// 4->5
	// 6->7 core1 rx7
	// 8->9 core1 rx9
	// 10->11 core rx11
	// 12->13 core rx13
	//
	for (bit16 i = 0; i < NUM_3D_TRI/2; i++)
	{
		data_in(Input_1); // stream 0 txs
		data_in(Input_1); // stream 0 txs
		data_redir_m(Input_1,Output_redir_odd,Output_redir_even); // 1 rx 2 4 tx
		rasterization2_m(Output_redir_odd,Output_r2_odd_toptx,Output_r2_odd_bottx,Output_redir_even,Output_r2_even_toptx,Output_r2_even_bottx);
	}


	// clean
	 Input_1.~stream();

	 Output_redir_odd.~stream();

	 Output_redir_even.~stream();

	 Output_r2_odd_toptx.~stream();
	 Output_r2_odd_bottx.~stream();

	 Output_r2_even_toptx.~stream();
	 Output_r2_even_bottx.~stream();

	 synchronize();

	 shutdown_ip();
}
