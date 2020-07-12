@page CodeDesign Code Design
<!-- @subpage subsubsystem1 -->
@tableofcontents

@section BenchmarkFunctions Benchmark Functions.

render_ps is a function that strictly is a fully software solution that does not use the hardware IP.

render_pl is a function that strictly uses PL to route the data. This function can only be used for the render.tcl but the resources consumption is very high

render_pl_mix is a function that uses a mixture of software and hardware streams. If a streams reads or writes data that lives on the same core it does not use the hardware IP it just talks directly to itself. hardware IP is used only for routing information from one core to another core. The hardware IP used is the compressed version which invokes a round robin scheduler to check each stream and push or pull data.

benchmark_ps_ps is a function that shows an example of reading and writing to streams from core to core. 

benchmark_pl_ps is a function that shows an example of reading and writing to streams from core to core using the hardware IP

benchmark_memory is a function that gives results for different memory configurations

@section ProperUsage ProperUseage

there are a few ways you can can use the IP's provided.

you can run them from software to software / hardware to software / software to hardware 

each has a slightly different configuration that may be dependent on your vivado design. I have tried to make it as intuitve as possible.

to declare a stream that transmits to the hardware call these API's

to decalre a stream that recives from the hardware call these API's

to bypass hardware you can configure a purely software stream using these API's

@section StreamAlgorithm Stream Algorithm Overview

The streams use a circular buffer to implement the transmission and recieving of data from hw to sw. A reader can read from the head pointer and modify the tail pointer where as the writer can read the tail pointer and modify the head pointer. In this manner we can ensure that no data is overwritten or read prematurely.

@section EnumsAndTypes Enums And Types

the streams class makes use of many enums to organize thoughts and types. They are briefly described below.

@subsection direction_t direction_t

this enums makes it verbose on if your IP is a transmitter or receiver. ie if you can call read or write. For the useage of a purely sw stream you can both read and write since no IP is associated.

```c
	enum direction_t
	{
		TX = 0,
		RX = 1,
		SW_SHARED,
	};
```

@subsection meta_data_t meta_data_t

this structure holds the location of the head and tail pointers as well as how large the stream is.

```c
	enum meta_data_t
	{
		POW_2 = BUFFER_SIZE_POW_2,
		RAW_BUFFER_SIZE = POW_2+2,
		HEAD_POINTER = POW_2+1,
		TAIL_POINTER = POW_2,
		MASK = POW_2-1,
	};
```
@subsection memory_t memory_t

this structure holds which type of memory the stream uses. You should only be using OCM for your application but to run benchmarks you can set to DDR or CACHE to see performance

```c	
	enum memory_t
	{
		DDR = 0, // data stored in DDR
		OCM = 1, // use OCM 
		CACHE = 2,
		INVALID_MEMORY,
	};
```

@subsection axi_port_t axi_port_t

this structure tells us how our IP is connected it is used purely for book keeping. 


```c
	enum axi_port_t
	{
		HPC0 = 0,
		HPC1,
		HP0,
		HP1,
		HP2,
		HP3,
		ACE,
		ACP,
		SW_PORT,
		INVALID_PORT,
	};
```

@subsection stream_id_t stream_id_t

this structure tells us what IPS are assocaited with each other. We can differentiate between receiver and transmitter ids and is useful for associating how things are connected.

```c
	// convert this to rx and tx enums
	enum stream_id_t{
		STREAM_ID_0 = 0,
		STREAM_ID_1 = 1,
		MAX_NUMBER_OF_STREAMS =20,
	};
```
@section ConfigurationParameters Configuration Parameters

the macros in the user_configs.h file allow you to easily tune parameters of your design without modifying much source. A brief explanation is of each parameter is below.

@subsection StreamDepth Changing Stream Depth

To change the stream depth one must configure 3 macros.

set the ```BUFFER_SIZE_POW_2``` in user_configs.h in.
set the enum metadata_t POW_2 to value in both the vivado HLS files and re synthesize.

the number set must match in all three files else the application will fail. This is because the two buffers connected will be modifying different head and tail pointers. The value used must also be a power of two.

@subsection SetAmountStreams Setting Amount Of HW Streams

set the amount of HW and SW streams your application needs. This will carve out your OCM memory regions in the init function allowing you to make little if no changes when you change from design to design. The HW streams macro pertains to not how many IP's you are using but the amount of fifo's that will connect to the BFT.

@section JumpTable JumpTable Overview

@subsection Interfacing Interfacing With The Jump table