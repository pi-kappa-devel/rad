## Synopsis

This is the top-level element-documentation of the code that accompanies the numerical section of the article 
[Costly Consideration and Dynamic Choice Set Formation](https://papers.ssrn.com/sol3/papers.cfm?abstract_id=3423876) 
(henceforth the article). 

The code implements a simple version of the radial attention model and solves it. For more details on the model's
technicalities see the [article](https://papers.ssrn.com/sol3/papers.cfm?abstract_id=3423876). The solver is based on a concurrent re-formulation of the value function iteration algorithm. Further, it uses an adaptive search grid implementation to provide more accurate optimal control approximations.

If you use this software, please consider citing it. The permanent link of the software's release version is 
[![DOI](https://zenodo.org/badge/doi/10.5281/zenodo.3353648.png)](https://zenodo.org/badge/latestdoi/199311654). 
Suggested citations can be found at this link. 

## Installation

The source code of the project can be found at this [repository](https://github.com/pi-kappa-devel/rad). The easiest way to build the C applications of the project is to use [CMAKE](https://cmake.org/). The following macros can be 
passed to CMAKE :
 - If you want to build with debug symbols and warnings use `CMAKE_BUILD_TYPE=Debug`.
 - If you want to build fully optimized for execution speed use `CMAKE_BUILD_TYPE=Release`.
 - If you want to have bound checking for the grids, set `GRID_T_SAFE_MODE=1`.
 - If you want the parameter maps to check if the passing keys exist, set `PMAP_T_SAFE_MODE=1`.
 - Lastly, if you want the compilation to include debugging development functionality use `RAD_DEBUG=1`.
 
CMAKE produces four targets; three executables and one documentation target. The last one gives this documentation.
The executable targets are
 - `rad_msol`  : Solves the radial attention model based on the saved parameterization file.
 - `rad_mcont` : Resumes the solution of the model that is halted in a previous execution. This is useful when you are using the code in environments with execution-time limits such as in clusters.
 - `rad_pardep`: Produces the data for the parameter dependence section of the 
 [article](https://papers.ssrn.com/sol3/papers.cfm?abstract_id=3423876).

The C code was compiled and tested using 
 - gcc version 4.8.5 20150623 (Red Hat 4.8.5-36) (GCC)
 - gcc version 8.3.0 (GCC)
 - gcc version 9.1.1 20190503 (Red Hat 9.1.1-1) (GCC)
 - Microsoft (R) C/C++ Optimizing Compiler Version 19.16.27025.1 for x86
 - Microsoft (R) C/C++ Optimizing Compiler Version 19.16.27025.1 for x64

The Python code was tested using 
 - Python 3.7.3 
 - Jupyter notebook 5.7.8

## Functionality

The C code is used to approximate the solutions of the radial attention model. It also stores the solution
and parameter analysis' binary data in the file system. The Python code is used to create model logic level objects
from the stored binary data. The notebook is using the resulting objects to produce the tables and the figures of
the [article](https://papers.ssrn.com/sol3/papers.cfm?abstract_id=3423876). The resulting notebook can be accessed
[here](https://rad.pikappa.eu/notebook.html) in an Html format.

## Concurrency

The concurrency is written on an operating system level using low-level abstractions (i.e. mutexes and locks). 
In UNIX based systems it is written using the POSIX Threads API [pthreads](http://www.cs.wm.edu/wmpthreads.html). 
In windows systems it uses the native windows 
[threading](https://docs.microsoft.com/en-us/windows/win32/api/processthreadsapi/) and 
[synchronization](https://docs.microsoft.com/en-us/windows/win32/api/synchapi/) APIs.

## Documentation

The documentation of the project can be found online [here](https://rad.pikappa.eu/index.html) and it is also available for [downloading](https://rad.pikappa.eu/refman.pdf) in a PDF format. It is built using [DOXYGEN](http://www.doxygen.nl/) and follows the `repeat your-self documentation approach´. 

Only the top-element functionality of the C code is documented. Drilling into lower-level functions requires that you have an understanding of the radial attention model. Knowledge of the standard value function iteration algorithm is another prerequisite. For the parallelization, basic knowledge of system multithreading implementations and the pipeline parallelization pattern is required. There are also some accompanying implementation comments in the source 
code that is not reported in this documentation.

The Python code is simple and its elements are briefly documented. The Jupyter notebook is also available with the documentation. 

## Contributors

Pantelis Karapanagiotis 

Feel free to join, share, contribute, distribute.

## License

The code is distributed under the MIT License. See the [LICENSE](https://rad.pikappa.eu/LICENSE.txt) file.
