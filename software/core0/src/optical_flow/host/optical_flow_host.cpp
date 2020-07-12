/*===============================================================*/
/*                                                               */
/*                    optical_flow_host.cpp                      */
/*                                                               */
/*      Main host function for the Optical Flow application.     */
/*                                                               */
/*===============================================================*/

// standard C/C++ headers
#include <cstdio>
#include <cstdlib>
#include <getopt.h>
#include <string>
#include <time.h>
#include <sys/time.h>

// other headers
#include "utils.h"
#include "typedefs.h"
#include "check_result.h"

#ifdef SDSOC
  // sdsoc headers
  //#include "sds_lib.h"
  // hardware function declaration
  #include "../sdsoc/optical_flow.h"
#endif
#ifdef SW
  # include "../optical_flow_sw.h"
#endif



int optical_flow(int argc, char ** argv)
{
  printf("Optical Flow Application\n");

// // sw version host code
//  #ifdef SW
//    static pixel_t frames[5][MAX_HEIGHT][MAX_WIDTH];
//    static velocity_t outputs[MAX_HEIGHT][MAX_WIDTH];
//
//    // use native C datatype arrays
//    for (int f = 0; f < 5; f ++ )
//      for (int i = 0; i < MAX_HEIGHT; i ++ )
//        for (int j = 0; j < MAX_WIDTH; j ++ )
//          frames[f][i][j] = imgs[f].Pixel(j, i, 0) / 255.0f;
//
//    // run
//    optical_flow_sw(frames[0], frames[1], frames[2], frames[3], frames[4], outputs);
//  #endif
//
//  // check results
//  printf("Checking results:\n");
//
//  check_results(outputs, refFlow, outFile);
//
//  // print time
//  long long elapsed = (end.tv_sec - start.tv_sec) * 1000000LL + end.tv_usec - start.tv_usec;
//  printf("elapsed time: %lld us\n", elapsed);

  return EXIT_SUCCESS;

}
