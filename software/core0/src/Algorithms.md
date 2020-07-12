\page Algorithms Algorithms
<!-- @subpage subsubsystem1 -->

\tableofcontents

\section Algorithms Algorithms

Two algorithms were explored for the efficacy of moving data at a high performance from PS to PL. Both the presence-bit scheme as well as a shared head and tail pointer scheme for implementing a circular buffer for sharing data were explored. 

\subsection Presence Presence

<!--  \begin{figure}[H]
	\centering
	\includegraphics[width=0.3\textwidth]{pics/presence_buffer.PNG}
	\caption{\small{Presence buffer}}
	\label{fig:fig12}
\end{figure} -->


This data passing scheme is used in the packet-switched network that uses the Butterfly Fat Tree (BFT) as an architecture. It has the advantage of being consistent with the algorithm in
the BFT as well as being a constant one read and one write. For a 32-bit data case a receiver would read 64 bits and check the 33rd bit. If the bit is high the lower 32 bits can be extracted and the 33rd bit can be cleared. If the bit is not high a read from global memory occurs again. On the sender side, the sender first checks the 33rd bit, if it is high then they must wait to write, if it is low they can write their 32 bits of data into the lower portion.

During the implementation, it was found that this algorithm was not favorable for high-performance data movement. On the hardware side, it is not possible to pipeline reads and writes. This is because to advance the pointer the 33rd, presence bit needs to be examined. This would be acceptable if the memory was in a BRAM but it is not, so the data must be retrieved from global memory, examined, and then the pointer can be advanced or not depending on if the bit is high or low. Consider a case where a user is reading data with a global-memory read latency of 10 cycles and write latency 10 cycles. The hardware must first wait 10 cycles to check the bit, if it is high, the data can then be extracted and a zero written to global memory which again takes 10 cycles. So in this example to perform one "presence read" the hardware is doing 20 cycles of serialized waiting which is not an efficient use of the hardware. This issue can be mitigated by implementing a pre-fetching scheme. This proved to be difficult to synthesize without causing stalls; furthermore, other data-passing schemes can solve this issue in a more graceful manner. The code for a presence write is shown below and the dependency can be seen with the read from global memory with the use of \texttt{this->ptr}.

<!-- \lstinputlisting[language=C,caption={\small{Presence\_Write}},label=PresenceWrite]{code/presence_write.txt} -->

On the PS side the presence-bit scheme is not as efficient because the bandwidth is effectively cut in half. If 32 bits of data are being moved it is not possible in software to declare a 33-bit data type as in hardware. In software the primitive 64-bit data type must be used which means 31 bits are wasted. For these reasons the presence-data passing scheme proved to not be a good fit for this application.

\subsection HeadTailPointer Head Tail Pointer

<!--  \begin{figure}[H]
	\centering
	\includegraphics[width=0.3\textwidth]{pics/circular.PNG}
	\caption{\small{Circular buffer}}
	\label{fig:fig13}
\end{figure} -->

The next algorithm that was explored and implemented is the circular buffer with a shared head and tail pointer. The head is only advanced by the writer and the tail is only advanced by the reader. In this way we can guarantee mutual exclusion between a single reader and single writer. The writer reads the tail to ensure it is safe to write. If it can write, the new data element is written and the head pointer is advanced to the next location. Likewise, on the other reader it can check the head pointer and compare it to his tail pointer. If they are not equal then there is data available to read. The reader can then read and advance its tail pointer. This has many advantages in hardware as it is now possible to pipeline the reads and writes. For example, on the reader side it is possible to check the head pointer, if it is 20 and our tail pointer is four that means it is safe to read 16 elements. Thus it is possible to pipeline 16 reads from global memory sequentially and push them into the BFT when they become available. In this manner it is feasible to achieve very high throughput from our PL.


<!-- \lstinputlisting[language=C,caption={\small{PL\_Circular\_Write}},label=PLCircularWrite]{code/circ_write.txt} -->

One interesting bug that was encountered while building this circular buffer was that there were times that the PL would read stale data. This was occurring because during a PS write the head pointer was being written before the new data was being written. This could have occurred because of some out-of-order operations in hardware or at the compiler level. To fix this issue an assembly level synchronization barrier \textbf{dsb();} had to be issued after the new data was written. This ensured that the data was written successfully before the head pointer was updated. This can be seen in \textit{listing 3 line 30}.
<!-- 
%\begin{figure}[H]
%	\centering
%	\includegraphics[width=0.8\textwidth]{pics/circ_write.PNG}
%	\caption{PS Circular Write}
%	\label{fig:fig16}
%\end{figure}   -->

<!-- \pagebreak -->

<!-- \lstinputlisting[language=C,caption={\small{PS\_Circular\_Write}},label=PSCircularWrite]{code/stream_read.txt} -->

\subsection Verification

For these streams to work two things must be true: (1) data is coherent between the PS and PL and (2) data is not being prematurely read or written. This can be a tricky thing to get right but easy to test. Four tests were created and are described below:

\li \c Test1: Two cores PS only:
In this test there is one core that is being strictly read and one core that strictly writes using the stream buffers.
	
<!-- 	\begin{figure}[H]
		\centering
		\includegraphics[width=0.3\textwidth]{pics/2corePS.png}
		\caption{\small{2-core test}}
		\label{fig:fig14}
	\end{figure} -->
	
\li \c Test2: Two cores PS and PL:
In this test core one writes to our PL block, this PL block pushes the data to the next PL block which writes it to our second core.

<!-- 	\begin{figure}[H]
		\centering
		\includegraphics[width=0.3\textwidth]{pics/2corePL.png}
		\caption\small{{2-core test}}
		\label{fig:fig15}
	\end{figure} -->
	
	
\li \c Test3: Four cores PS only:
In this test there is the same data-passing scheme as before but now it is extend to four cores. Note that two of the cores will perform a stream read \textbf{AND} a stream write.
	
<!-- 	\begin{figure}[H]
		\centering 
		\includegraphics[width=0.7\textwidth]{pics/4corePS.PNG}
		\caption\small{{4-core PS test}}
		\label{fig:fig16}
	\end{figure} -->

\li \c Test4 Four cores PS and PL:
In this test case there is the same data-passing scheme as before but it is extended to four cores with PL blocks in between. Again, note that two of the cores will perform a stream read and stream write.
	
<!-- 	\begin{figure}[H]
		\centering
		\includegraphics[width=0.7\textwidth]{pics/4corePL.PNG}
		\caption{\small{4-core PL test}}
		\label{fig:fig17}
	\end{figure}
 -->	

In all test cases one writer will write a unique number and the other cores will assert that the data read is what is expected. Simply put, the writer will write one then two then three and the reader will expect to read one, two, three. In this manner it is ensured that the transactions are coherent and not premature.