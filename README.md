# CAD Project 1
This repository contains the source code for our first CAD project.</br>

The repository is organized as follows:

* `Source Code` - A folder containing the source code for the different versions of our implementations. More information in the [Implementations](#Implementations) section of this readme.  
* `Tester` - A folder containing the source code for the program that compiles, tests, and outputs the results of every single one of our implementations into a single, easily parsable text file. Compiling this program is as simple as calling make, and executing it is just a matter moving to the output folder (which should be the `Source Code` folder, and running the program.  
* `Report` - A folder containing our report, both as a `.docx` and as a `.pdf`.</br></br>

### Implementations

Inside the `Source Code` folder, you can find many different versions of the program as we iterated on it. Compiling each version is as simple as going to the version's directory and calling `make`. Running the programs is as simple as calling `./program image.ppm`.</br>

You can compile all the versions at once by running the `makeall.sh` shell script.</br>

There are 5 main folders, named `Images`, `Original`, `Single Block`, `Multi Block`, and `Shared Memory`. 

* `Images` - A collection of .ppm images to test the compiled programs with.
* `Original` - The original code, before any improvements are performed. This isn't a 1 to 1 copy of the original source code provided by our professor, it was instead rewritten to make further modifications a lot simpler. Of note, we modified the image data to be organized in a struct instead of an array, and we modified the blur matrix kernel to be calculated manually. This allows for a blur of any size to be applied by modifying the `KERNELRADIUS` macro. The blur and desaturation algorithms themselves, however, are unchanged and copied verbatim from the original source code to not influence the final time results.
* `Single Block` - Each of our implementations done with the CUDA blocks being calculated in a single dimension.
* `Multi Block` - Each of our implementations done with the CUDA blocks being calculated in two dimensions.
* `Shared Memory` - Each of our implementations done with the CUDA blocks being calculated in a single dimension and utilizing shared memory.</br></br>


Each of our implementations are stored in their own folder inside either `Single Block`, `Multi Block`, or `Shared Memory`. The versions of each program are as follows:

##### V01 - CUDA
This is a very basic port of the code to CUDA. Time is now provided with and without the memory copying operations.

##### V02 - New Algorithms
In this version, the algorithms were modified to reduce having to perform `if` checks and to remove unecessary calculations.

##### V03 - Memory Alignment
In this version, the image data structs were modified for 32-Bit memory alignment.

##### V04 - Combined Algorithms
In this version, both the blur and desaturation algorithms were combined into a single function.

##### V05 - Multipass Blur
In this version, the blur algorithm was reduced from O(N^2) to O(2N) by leveraging the fact that image blurring kernels are seperable. Only benefitial for large blurring matricies.

##### V06 - Streams (UNIMPLEMENTED)
In this version, CUDA was set up to utilize streams.