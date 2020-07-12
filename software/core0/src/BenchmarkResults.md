\page BenchmarkResults BenchmarkResults
<!-- @subpage subsubsystem1 -->

\tableofcontents

\section PSBenchmarks PS Benchmarks

Similarly to the PL memory benchmarks, it is important to note how fast data can be transferred in the  PS to better understand where the bottlenecks may lie. Using the Xilinx board-support packages memory region can be declared as \textbf{non-cacheable} or \textbf{outer-shareable}. Outer-shareable means that the data cannot live in the L1 cache; the highest memory subsystem that the data can live in is the L2 cache. \textbf{Non-cacheable} means that the data cannot live in any cache system. Memory marked as \textbf{outer-shareable} allows the ACP and HPC ports to make coherent transactions as these ports can snoop the L2 cache. 

Initially, three different tests were created to see the bandwidth of the PS. A simple read and write were tested where a program reads from or writes to a circular buffer. This involves reading, incrementing a pointer, and checking for wrap-around. The buffer is placed in OCM, the L2 cache, or main memory. The performance is illustrated in \textit{figure 8}.

<!-- \begin{figure}[H]
	\centering
  \includegraphics[width=0.8\textwidth]{pics/PSThroughputResults.png}
  \caption{\small{PS throughput in gbps}}
  \label{fig:fig8}
\end{figure} -->


As seen in \textit{figure 8}, writing to OCM memory, reading from the L2 cache, and writing to the L2 cache are high-performance options. As expected, writing to main memory is very expensive. There is one unexpected condition; reading from OCM happens to be very expensive relative to writing -- 0.5 gbps compared to ~4 gbps.

Taking it a step further, the code is then manually unrolled by a factor of two and again by 60. The code was also converted to issue vector loads and stores to see what optimizations can be achieved. Unrolling by a factor of 60 drastically increased throughput (See Fig.~\ref{fig:fig11}). \textit{Figure 9} shows the assembly code for an unrolled write. As shown in (\textit{figure9}) the assembly code is issuing loads and adds. Dependencies are removed and higher throughput can be achieved relative to our previous case.

<!-- \begin{figure}[H]
	\centering
	\includegraphics[width=0.5\textwidth]{pics/asm_write.PNG}
	\caption\small{{Unrolled assembly}}
	\label{fig:fig9}
\end{figure} -->

 Surprisingly, unrolling by a factor of two versus a factor of 60 had no impact on the throughput of reading from OCM. However, a bottleneck still exists here, and there appears to be some hardware dependency that serializes the read. The only way to see some further performance gain is issuing a 128-bit vector load, which doubles the performance as one would expect going from 64 to 128 bits. 
 
<!--  \begin{figure}[H]
 	\centering
 	\includegraphics[width=0.8\textwidth]{pics/PSOCMREAD.png}
 	\caption{\small{On-chip memory read}}
 	\label{fig:fig10}
 \end{figure} -->

Vector loads and stores can then be compared to unrolling by a factor of 2 where 128 bits are moved. And as expected, unrolling with a higher factor leads to a higher throughput.
 

<!--  \begin{figure}[H]
 	\centering
	\includegraphics[width=0.8\textwidth]{pics/PSThroughput_accel.png}
	\caption{\small{Unrolled, vector results}}
	\label{fig:fig11}
\end{figure} -->

With these benchmarks in mind it was decided to implement the buffers in on-chip memory and ACP. The chip-memory showed a high throughput solution for both the PS and PL with a uniform solution to work on both the R5 and A53 processors.


\section PLBenchmarks PL Benchmarks

There are three main types of memory in our Xilinx SoC: main memory, coherent memory in a cache, and on-chip memory. All of these memories were benchmarked with 32-, 64-, and 128-bit interfaces to capture what is the best way to move data from PS to PL or PL to PS. 

For a PL &rarr; memory &rarr; PL loop-back test, two blocks were created in Vivado HLS, one block that reads from memory and pushes it through an AXI stream to the other block. The second block reads from the AXI stream and writes the data to memory. It is also possible to synthesize separate test blocks, read-only and write-only IP blocks to see where in this loop-back test the bottleneck may lie; reading from memory or writing to memory. With this information it is possible to get a sense of what performance can be achieved from the shared buffers.

\textit{Figure 6} shows the sequence of events that occur during a loop-back test. The PS orchestrates the PL’s data region and begins the test. The PL is programmed to read sequentially from a memory region and pass the data onto another PL who writes it to memory.

<!-- \begin{figure}[H]
	\centering
  \includegraphics[width=0.8\textwidth]{pics/sequence.png}
  \caption{\small{Loop back test}}
  \label{fig:fig6}
\end{figure} -->

\textit{Figure 7} shows that if non-cacheable memory is used the HP port going through on-chip memory has the highest throughput. If the goal is coherent memory then the ACP port can be utilized to achieve the highest throughput. The caveat with the ACP port is that the data transfers must be 16-byte aligned. This port also resides in the L2 memory sub-system of the A53s. Because of the port's placement in the L2, the R5 cores will not be able to utilize this port. Note that since the ACP port requires 16-byte aligned pointers only 128-bit data transfers were benchmarked.

<!-- \begin{figure}[H]
	\centering
  \includegraphics[width=0.8\textwidth]{pics/PLLoopBackThroughput.png}
  \caption{\small{Loop back test}}
  \label{fig:fig7}
\end{figure} -->

Figure 7 shows that with the ACP port using cacheable memory and 128-bit data movement throughput around 5.8gbps is observed. Another high performance option, is achieved using non-cacheable memory in on chip memory.  ~3.8 gbps were observed in this experiment. Enabling coherency in the HPC ports proved to yield lower than expected performance with
performances around only 0.5gbps with cacheable memory. This result is a bit of a surprise considering the ACP port can provide such high throughput.

\section AlgorithmResults Algorithm Results

In the test cases shown below both algorithms run using on-chip memory and move in a data-flow manner from one core to the next as depicted in the verification section. As seen in the data-presence scheme, when the PL blocks are added some throughput is lost. It can be inferred that the bottleneck resides in the PL. When looking at the head and tail pointer scheme, adding the PL has no effect on the throughput. This shows that the bottleneck lies in the PS. Our raw throughput is computed by taking the minimum the PL PS data movement from the previous benchmarks. Comparing the
OCM data movement we get (min(0.5,3.8))=0.5 coming from an OCM read and 3.8 coming from the PL loopback test.  The data passing algorithm achieves about 80\% of the possible throughput for both the OCM and the
coherent ACP options. ~80\% makes sense since a single 'stream read' consists of a read and write to memory. In the worst-case scenario when the pointers collide an additional read is issued. Since a stream read has the possibility of issuing two reads this is where some drop in performance exists. Higher throughput can be achieved by extending the stream buffers to 1k or 2k. This would result in fewer collisions, thus fewer reads from memory.

<!-- \begin{figure}[H]
	\centering
  \includegraphics[width=0.8\textwidth]{pics/AlgorithmThroughput.png}
  \caption{\small{PS throughput in gbps}}
  \label{fig:fig18}
\end{figure} -->

\subsection RenderResults Render Results