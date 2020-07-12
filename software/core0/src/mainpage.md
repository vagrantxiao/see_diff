\mainpage PR_Flow DIRC-Gentle Dataflow Extension

\tableofcontents

**University of Pennsylvania IC group PR_Flow Stream Extension**

**Maintainer/Creator Eric Micallef**
* https://www.linkedin.com/in/eric-micallef-99291714b/
* emicallef25@gmail.com
* Tested on 2018.3 vivado
* Tested on u96v2

\section Introduction Introduction

Dataflow Incremental Refinement of C, or Dirc Gentle, is an extension of a new and exciting research project PR Flow created by Yuanlong Xiao PR Flow requires users to fit their application into a small page size with fixed hardware requirements. This will usually lead to users needing to change how their program runs. Instead of one big operator, PR Flow breaks the operators into smaller operators that have a producer-consumer, or dataflow, interface between them. Since debugging in hardware is difficult and time consuming this is where the Dirc Gentle project comes in.

Dirc Gentle provides a layer of abstraction that allows developers to not worry about whether their software is running in software or hardware. With this abstraction developers can begin breaking down their program into smaller logical operators that will fit into a fixed page size that PR flow defines. Users can run everything on software using the streams created from this project. When a developer has a design they like and one that fits the resource requirements for PR flow, they can then move this logic to hardware. A powerful element of these streams is that they can keep the dataflow paradigm that PR Flow encourages whether an operator is in software or hardware. 

Whether there is a design where all the operators are in hardware (figure 1) or where task A and task C are in hardware and task B is in software (figure 2), Dirc Gentle helps maintain the dataflow parallelism between operators making debugging easier and more realistic. 

<!-- \begin{center}

	\begin{figure}[H]
  	\centering
  	\includegraphics[width=0.6\textwidth]{pics/hardware.png}
  	\caption{\small{Hardware dataflow}}
  	\label{fig:fig1}
	\end{figure}

	\begin{figure}[H]
	\centering
  	\includegraphics[width=0.6\textwidth]{pics/hwsw.png}
	\caption{\small{Hardware and software dataflow design}}
  	\label{fig:fig2}
	\end{figure}

\end{center} -->

Currently the movement of operators is not possible in SDSoC or Vivado HLS. In SDSoC a new test bench must be built and hardware tasks can only run sequentially. Each hardware logic block must have a DMA interface to move the data in and out which can consume a significant amount of hardware resources. The execution could be made parallel using the async and wait pragmas but this becomes challenging and may introduce new obstacles that are not coherent with the original design. Likewise, Vivado HLS runs everything serially, this can lead to refactoring some of the design as well. The \textit{Dirc Gentle} streams mitigate all of these problems by allowing the developer to test his or her design in the parallel environment on hardware immediately.


\subsection Goals What we want to achieve

To communicate from Programmable Software (PS) to Programmable Logic (PL) or PL to PS only a one way stream is needed (\textit{figure 3}). A FIFO buffer is created where there exists one reader or writer to this buffer. With this buffer in place data can be shared between software and hardware. At a high level, readers read and return data when it is available and writers write and return when space becomes available.

<!-- \begin{figure}[H]
	\centering
  \includegraphics[width=0.6\textwidth]{pics/fifo_buffer.png}
  \caption{\small{A shared FIFO depicting one-way data movement between PS and PL}}
  \label{fig:fig3}
\end{figure} -->


\subsection AXIports AXI Ports

AXI ports, also know as AXI channels, can offer a high-bandwidth solution for sharing data between PS and PL. There exists on the ultra96 board four high-performance ports (HP), two high-performance coherency ports (HPC), one accelerator port (ACP), and one AXI coherency extension (ACE) port. Each has its own unique attributes. HP streams are not cache-coherent. HPC ports can be configured to be cache-coherent and pass through the CCI. The single ACP and ACE ports reside in the memory sub-system allowing for coherent transactions. ACE is two-way coherent and can provide low-latency cache-to-cache transfers but Vivado HLS does not create any IP that can talk using the ACE protocol. Thus, the HP, HPC, and ACP ports were all benchmarked and tested. 

<!-- \begin{figure}[H]
	\centering
  \includegraphics[width=0.8\textwidth]{pics/axi.jpeg}
  \caption{\small{AXI interfaces, their data widths, and coherency}\cite{pspl}}
  \label{fig:fig5}
\end{figure} -->

\subsection Contents Contents

This readme will guide you through the creation of the benchmark for the stream IP used in pr_flow with some example useage as well as some information that may be useful for using vivado HLS IP 

\subpage BenchamarkRecreation \n
\subpage CodeDesign \n
\subpage BenchmarkResults \n
\subpage Algorithms \n
\subpage ResourceUtilization \n
\subpage HLSTips \n
\subpage Wishlist \n

\subsection References References

I used many references including the forums... here are some that I found helpful

https://www.xilinx.com/html_docs/xilinx2017_4/sdaccel_doc/jit1504034365862.html https://forums.xilinx.com/t5/High-Level-Synthesis-HLS/m-axi-interfaces-optional-depth-Offset-types/m-p/1069538#M19564 https://www.xilinx.com/support/answers/69446.html https://community.arm.com/developer/ip-products/processors/f/cortex-a-forum/3238/the-exact-definition-of-outer-and-inner-in-armv7 https://www.xilinx.com/support/documentation/sw_manuals/xilinx2017_4/ug902-vivado-high-level-synthesis.pdf https://www.xilinx.com/support/documentation/user_guides/ug1085-zynq-ultrascale-trm.pdf https://www.zynq-mpsoc-book.com/wp-content/uploads/2019/04/MPSoC_ebook_web_v1.0.pdf http://infocenter.arm.com/help/topic/com.arm.doc.ddi0470i/DDI0470I_cci400_r1p3_trm.pdf
