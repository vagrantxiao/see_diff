/*
 * gradient_z_calc.cpp
 *
 *  Created on: May 31, 2020
 *      Author: emica
 */
#include "../optical_flow.h"
#ifdef SW_COMPILE


// compute z gradient
void gradient_z_calc_sw_op(pixel_t frame0[MAX_HEIGHT][MAX_WIDTH],
                     pixel_t frame1[MAX_HEIGHT][MAX_WIDTH],
                     pixel_t frame2[MAX_HEIGHT][MAX_WIDTH],
                     pixel_t frame3[MAX_HEIGHT][MAX_WIDTH],
                     pixel_t frame4[MAX_HEIGHT][MAX_WIDTH],
                     pixel_t gradient_z[MAX_HEIGHT][MAX_WIDTH])
{
  for (int r = 0; r < MAX_HEIGHT; r ++)
  {
    for (int c = 0; c < MAX_WIDTH; c ++)
    {
      gradient_z[r][c] = 0.0f;
      gradient_z[r][c] += frame0[r][c] * GRAD_WEIGHTS[0];
      gradient_z[r][c] += frame1[r][c] * GRAD_WEIGHTS[1];
      gradient_z[r][c] += frame2[r][c] * GRAD_WEIGHTS[2];
      gradient_z[r][c] += frame3[r][c] * GRAD_WEIGHTS[3];
      gradient_z[r][c] += frame4[r][c] * GRAD_WEIGHTS[4];
      gradient_z[r][c] /= 12.0f;
    }
  }
}

#endif
