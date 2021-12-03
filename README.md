# CAD Project 1
This repository contains the source code for our first CAD project. </br>

The repository is organized as follows:
* `Source Code` - A folder containing the source code for the different versions of our implementations. More information in the [Implementations](#Implementations) section of this readme.

### Implementations

Inside the `Source Code` folder, you can find many different versions of the program as we iterated on it. Compiling each version is as simple as calling `make`, and running them is as simple as calling `./program image.ppm`. </br>

The versions are as follows:

##### Original
This is the original code, before any improvements are performed. This isn't a 1 to 1 copy of the original source code provided by our professor, it was instead rewritten to make further modifications a lot simpler. Of note, we modified the image data to be organized in a struct instead of an array, and we modified the blur matrix kernel to be calculated manually. This allows for a blur of any size to be applied by modifying the `KERNELRADIUS` macro. </br>

The blur and desaturation algorithms themselves, however, are unchanged and copied verbatim from the original source code to not influence the final time results.

##### V01 - CUDA
This is a very basic port of the code to CUDA. Time is now provided with and without the memory copying operations.

##### V02 - New Algorithms
In this version, the algorithms were modified to reduce having to perform `if` checks and to remove unecessary calculations.

##### V03 - Memory Alignment
In this version, the image data structs were modified for 32-Bit memory alignment.

##### V04 - Combined Algorithms
In this version, both the blur and desaturation algorithms were combined into a single function.

##### V05 - Multipass Blur (UNFINISHED)
In this version, the blur algorithm was reduced from O(N^2) to O(2N) by leveraging the fact that image blurring kernels are seperable. 