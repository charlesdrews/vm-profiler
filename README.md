vm-profiler
===========

This repository contains a virtual machine profiler written in C for a school assignment. The profiler generates basic block profiles and a control flow graph from a target application's instruction address stream. It is intended as a demonstration of a profiling feature that could be added to a hypothetical virtual machine manager.

Please see ***Drews_Phase3.pdf*** for a detailed report of the goals, methodology, and results of this project. I received a grade of "A" for this assignment.

Please see ***User_Manual.pdf*** for detailed descriptions of how to compile and run the executable, how to create the necessary address stream input files, and the format of the output files. An additional option for compiling is to use the included makefile.

This project relies on the [Valgrind](http://valgrind.org/) tool to create the necessary address stream input files.
