/*
 * tensor_weight_y.cpp
 *
 *  Created on: May 31, 2020
 *      Author: emica
 */
#include "../optical_flow.h"

#ifdef SW_COMPILE


// tensor weight y
void tensor_weight_y_sw_op(outer_t outer[MAX_HEIGHT][MAX_WIDTH],
                     tensor_t tensor_y[MAX_HEIGHT][MAX_WIDTH])
{
  for (int r = 0; r < MAX_HEIGHT + 1; r ++)
  {
    for(int c = 0; c < MAX_WIDTH; c ++)
    {
      tensor_t acc;
      for (int k = 0; k < 6; k ++)
      {
        acc.val[k] = 0;
      }

      if (r >= 2 && r < MAX_HEIGHT)
      {
        for (int i = 0; i < 3; i ++)
        {
          for(int component = 0; component < 6; component ++)
          {
            acc.val[component] += outer[r-i][c].val[component] * TENSOR_FILTER[i];
          }
        }
      }
      if (r >= 1)
      {
        tensor_y[r-1][c] = acc;
      }
    }
  }
}

#endif
