@page HLSTips HLS Tips
<!-- @subpage subsubsystem1 -->
@tableofcontents

@section HLSGeneratedBSP Using HLS Generated BSP for IP

```c
	// Below are C generated drivers provided by Vivado HLS. When you choose the 
	// s_axilite interface you will get a set of drivers to communicate to the PL
	// depending on the name of your application it may be called something slightly different rather than
	// XExample. the IsReady queries the PL to see if it can accept input
	// Set_input() tells the PL where to read from memory this is important. it sets the ptr thatI have called in my application 'input' 
	// Set_ouput() does the same.
	// the IsDone() function allows you to poll your PL to see when it has exited the HLS function you created
	// the start function tells the PL to begin running
	// more information on the generated API's can be found here: 
	// https://www.xilinx.com/support/documentation/sw_manuals/xilinx2017_4/ug902-vivado-high-level-synthesis.pdf page 503
    printf("XExample_IsReady() %d\n",XExample_IsReady(AXI_Config));
    XExample_Set_input_r(AXI_Config,(u32)input);
    printf("XExample_IsReady() %d\n",XExample_IsDone(AXI_Config));
    XExample_Set_output_r(AXI_Config,(u32)output);
    XExample_Start(AXI_Config);
```

```c
void example(volatile int *input, volatile int* output){

// this pragma defines a master interface to be created the offset slave is important because it allows us to control from the PS where these pointers point at
#pragma HLS INTERFACE m_axi port=input offset=slave bundle=gmem0
#pragma HLS INTERFACE m_axi port=output offset=slave bundle=gmem1
// The s-axilite interface allows us to create a port which can set the pointers, start the PL query the PL and have some interaction with it
//
#pragma HLS INTERFACE s_axilite port=input bundle=control
#pragma HLS INTERFACE s_axilite port=output bundle=control
#pragma HLS INTERFACE s_axilite port=return bundle=control

// More information can be found here:
// https://www.xilinx.com/html_docs/xilinx2017_4/sdaccel_doc/jit1504034365862.html
// https://forums.xilinx.com/t5/High-Level-Synthesis-HLS/m-axi-interfaces-optional-depth-Offset-types/m-p/1069538#M19564
// 
```

@section CacheCoherentIP Creating Cache Coherent AXI and IP

```c
    // sets the addresses to outershareable so the PL can see the data
    Xil_SetTlbAttributes(input, 0x605);
    Xil_SetTlbAttributes(output, 0x605);
    // enables snooping for the HPC0 and HPC1 ports
    Xil_Out32(0xFD6E4000,0x1);
    dmb();
    // More information can be found here:
    // https://www.xilinx.com/support/answers/69446.html
    // https://community.arm.com/developer/ip-products/processors/f/cortex-a-forum/3238/the-exact-definition-of-outer-and-inner-in-armv7
```

Important you must set the IP CACHE bits high (1111)

![](../pics/coherent_ip.PNG)

@section UsingOCM Configuring Vivado to use OCM

By default vivado will exclude OXM from the AXI regions to include them it is rather simple. You may just add your IP and after connecting it to an AXI port go to the address editor, right click and say include region.

![](../pics/OCM_config.PNG)

@section Resources Resources

Current reource utilization for one IP with 6 streams synthesized in vivado is.

| Resource | Amount            | Percent        |  
| -------  | ----------------- | ---------------|  
| LUT      |              5501 |           7.80 |  
| LUTRAM   |               727 |           2.52 |  
| FF       |              9147 |           6.48 | 
| BRAM     |              5.50 |           2.55 |  
| BUFG     |                 1 |            .51 |  