@page BenchamarkRecreation Benchmark Recreation
<!-- @subpage subsubsystem1 -->
@tableofcontents

<!-- # Benchmark Recreation -->

@section VivadoHLS Vivado HLS

1. create a HLS project and synthesize and export the RTL projects using the cpp files in the hardware->vivado_hls folder.

	* Select the chip: xczu3eg-sbva484-1-e
	* Set the freqenucy to 300MHz

2. You will need to create three separeate solutions. Synthesize and export each RTL to the corresping solution.

3. your project could look something like this where circ_buff_read is the current active solution to be synthesized and exported.

![](../pics/hls_project.PNG)

@section Vivado Vivado

1. create a new project in vivado.

2. add the IP created by HLS. This can be done by settings->IP->repository->add

![](../pics/ip_repo.PNG)

3. run the tcl script. This can be done by running "source <path_to_tcl>/render_compressed.tcl"

![](../pics/vivado_source.PNG)


4. once the project is built click "generate bitstream". Your project should look something like below.

![](../pics/board_design.PNG)

5. wait for bitstream to complete...

6. once completed go to file->export->export hardware (board design must be open for this option to be available) check the include bitstream box

7. launch SDK. file->launchSDK

8. create four applications each with C++ and a BSP with a different core. You can change the core by changing the processor drop down. 

![](../pics/new_application_project.PNG)

9. copy the src over to the corresponding core.

10. Highlight all projects and do a clean build

11. run all 4 cores. Be sure to check the boxes below and set all 4 cores to run the correct elf.

![](../pics/run_config1.PNG)

![](../pics/elf_config.PNG)