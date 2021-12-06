#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <time.h>
#include <ctype.h>
#include <string.h>
#include <math.h>
#include <cuda.h>
#include <cuda_runtime.h>


/*********************************
              Macros
*********************************/

// Number of CUDA streams
#define STREAMCOUNT  4

// Print the image before and after to the console
#define PRINT_PPM    0

// The radius of the blur matrix kernel
#define KERNELRADIUS 2

// Useful constants. Don't touch
#define KERNELSIZE   ((2*KERNELRADIUS)-1)
#define TWO_PI 6.28319
#define EULER 2.71828


/*********************************
            Structures
*********************************/

// Texel struct
typedef uint8_t color;
typedef struct {
    color r;
    color g;
    color b;
    color alignment; // Unused, exists for 32 bit memory alignment.
} texel;

// Blur matrix struct (workaround for passing arrays by value)
typedef struct {
  float data[KERNELSIZE];
} blurMatrix;


/*==============================
    read_ppm
    Reads a PPM image ascii file
    @uint32_tparam The pointer to the image file handler 
    @param The pointer to an array of texels
    @param A pointer to store the image width 
    @param A pointer to store the image height
    @param A pointer to store the size of each texel 
==============================*/

void read_ppm(FILE* f, texel** img, uint32_t* width, uint32_t* height, uint32_t* texelsize)
{
    color r, g, b;
    int c;
    uint32_t count = 0;
    char ppm[10];

    // Read the header to get the image properties
    while ((c = fgetc(f)) != EOF && count < 4)
    {

        // Ignore whitespace
        if (isspace(c)) 
            continue;

        // Ignore comments
        if (c == '#') 
        {
            while (fgetc(f) != '\n')
                ;
            continue;
        }

        // Read header elements
        ungetc(c, f);
        switch (count) 
        {
            case 0: count += fscanf(f, "%2s", ppm); break;
            case 1: count += fscanf(f, "%u%u%u", width, height, texelsize); break;
            case 2: count += fscanf(f, "%u%u", height, texelsize); break;
            case 3: count += fscanf(f, "%u", texelsize);
        }
    }

    // Validate what we read
    assert(c != EOF);
    assert(!strcmp("P3", ppm));

    // Allocate memory for the image data
    *img = (texel*)malloc((*width)*(*height)*sizeof(texel));
    assert(img != NULL);

    // Read the texel data from the file
    int pos = 0;
    while (fscanf(f,"%hhu%hhu%hhu", &r, &g, &b) == 3)
    {
        texel* t = &(*img)[pos];
        t->r = r;
        t->g = g;
        t->b = b;
        pos++;
    }
    assert(pos == (*width)*(*height));
}


/*==============================
    write_ppm
    Writes a PPM image ascii file
    @param The pointer to the image file handler 
    @param The image data
    @param The image width (in texels)
    @param The image height (in texels)
    @param The size of each texel 
==============================*/

void write_ppm(FILE* f, texel* img, const uint32_t width, const uint32_t height, const uint32_t texelsize)
{
    // Write the header
    fprintf(f, "P3\n%u %u %u\n", width, height, texelsize);

    // Write the texel data
    for (uint32_t l=0; l<height; l++)
    {
        for (uint32_t c=0; c<width; c++) 
        {
            uint32_t p = (l*width + c);
            fprintf(f, "%hhu %hhu %hhu  ", img[p].r, img[p].g, img[p].b);
        }
        putc('\n', f);
    }
}


/*==============================
    write_ppm
    Prints a PPM image to the screen
    @param The image width (in texels)
    @param The image height (in texels)
    @param The image data
==============================*/

void printImg(const uint32_t imgw, const uint32_t imgh, const texel* img)
{
    for (uint32_t j=0; j<imgh; j++) 
    {
        for (uint32_t i=0; i<imgw; i++) 
        {
            uint32_t x = (i+j*imgw);
            printf("%hhu,%hhu,%hhu  ", img[x].r, img[x].g, img[x].b);
        }
        putchar('\n');
    }
    putchar('\n');
}


/*==============================
    shader_pass1
    Performs a vertical gaussian blur with CUDA
    @param The image array to output to
    @param The image array to read from
    @param The stream offset
    @param The size of the image (in texels)
    @param The image width (in texels)
    @param The image height (in texels)
    @param The gaussian blur matrix
==============================*/

__global__ void shader_pass1(texel* out, texel* in, const uint32_t offset, const uint32_t size, const uint32_t width, const uint32_t height, const blurMatrix filter)
{
    // Calculate the texel coordinate that this thread will modify
    const uint32_t index = offset + blockIdx.x*blockDim.x + threadIdx.x;

    // Ensure we don't go out of bounds
    if (index >= size)
        return;

    // Calculate the texel coordinates
    float r=0, g=0, b=0, n=0;
    const int idx = index%width;
    const int idy = index/width;
    const uint32_t uppery = width*min((idy+(KERNELRADIUS-1)), height);

    // Perform a first vertical gaussian blur pass
    for (int y=max(0, ((int)width)*(idy-(KERNELRADIUS-1))), k=-min(0, idy-(KERNELRADIUS-1)); y<uppery; y+=width)
    {
        const float scale = filter.data[k++];
        const texel* ti = &in[idx+y];
        r += scale*ti->r;
        g += scale*ti->g;
        b += scale*ti->b;
        n += scale;
    }
    n = 1/n;

    // Store this first pass
    texel* to = &out[index];
    to->r = r*n;
    to->g = g*n;
    to->b = b*n;
}


/*==============================
    shader_pass2
    Performs a horizontal gaussian blur, and then desaturation with CUDA
    @param The image array to output to
    @param The stream offset
    @param The size of the image (in texels)
    @param The image width (in texels)
    @param The image height (in texels)
    @param The gaussian blur matrix
    @param The amount of saturation (ranging from 0 to 1).
    @param The amount of desaturation (ranging from 0 to 1).
==============================*/

__global__ void shader_pass2(texel* img, const uint32_t offset, const uint32_t size, const uint32_t width, const uint32_t height, const blurMatrix filter, const float saturation, const float desaturation)
{
    // Calculate the texel coordinate that this thread will modify
    const uint32_t index = offset + blockIdx.x*blockDim.x + threadIdx.x;

    // Ensure we don't go out of bounds
    if (index >= size)
        return;

    // Calculate the texel coordinates
    float r=0, g=0, b=0, n=0;
    const int idx = index%width;
    const int idy = index/width;
    const uint32_t upperx = min(idx+(KERNELRADIUS-1), width);

    // And now a second horizontal pass
    const uint32_t ystart = idy*width;
    for (int x=max(0, idx-(KERNELRADIUS-1)), k=-min(0, idx-(KERNELRADIUS-1)); x<upperx; x++)
    {
        const float scale = filter.data[k++];
        const texel* ti = &img[x+ystart];
        r += scale*ti->r;
        g += scale*ti->g;
        b += scale*ti->b;
        n += scale;
    }
    n = 1/n;

    // Update the output image's desaturated + blurred texel value 
    const color nr = r*n, ng = g*n, nb = b*n;
    const float grey = saturation*(0.3*nr + 0.59*ng + 0.11*nb);
    texel* to = &img[index];
    to->r = desaturation*nr + grey;
    to->g = desaturation*ng + grey;
    to->b = desaturation*nb + grey;
}


/*==============================
    gaussian
    Calculates a coordinate on the gaussian matrix
    @param The gaussian matrix X coordinate
    @param The gaussian matrix Y coordinate
==============================*/

float gaussian(const float x, const float y) // I tried to inline this, but apparently GCC on linux doesn't behave the way I expected...
{
    const float sigma = KERNELRADIUS/2;
    const float sigmasqu = sigma*sigma;
    const float mean = KERNELSIZE/2;
    return exp(-0.5*(pow((x-mean)/sigma, 2.0) + pow((y-mean)/sigma, 2.0)))/(TWO_PI*sigmasqu);
}


/*==============================
    main
    Program entrypoint
    @param The number of arguments to the program
    @param A list of strings with the program arguments
==============================*/

int main(int argc, char* argv[]) 
{
    struct cudaDeviceProp properties;
    uint32_t streamSize;
    uint32_t threadCount, blockCount, streamCount;
    uint32_t imgh, imgw, imgsize;
    uint32_t texelsize;
    texel* img;
    float saturation = 0.5; // default value
    blurMatrix filter;

    // Check if program arguments exist
    if (argc != 2 && argc != 3)
    {
        fprintf(stderr, "usage: %s img.ppm [saturation]\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    // Get the saturation argument if it exists
    if (argc == 3)
        saturation = atof(argv[2]);

    // Open the image file
    FILE *f = fopen(argv[1], "r");
    if (f == NULL) 
    {
        fprintf(stderr, "can't read file %s\n", argv[1]);
        return EXIT_FAILURE;
    }

    // Read the image file
    read_ppm(f, &img, &imgw, &imgh, &texelsize);
    printf("PPM image %ux%u. Texel size = %u\n", imgw, imgh, texelsize);
    imgsize = imgw*imgh;
    #if PRINT_PPM
        printImg(imgw, imgh, img);
    #endif
    fclose(f);

    // Calculate the gaussian blur matrix
    for (uint32_t x=0; x<KERNELSIZE; x++)
        filter.data[x] = gaussian(x, KERNELRADIUS-1);

    // Setup CUDA
    cudaGetDeviceProperties(&properties, 0);
    threadCount = fmin(imgsize, properties.maxThreadsPerBlock);

    // Allocate memory for the outputted image data buffer
    texel *out = (texel*)malloc(imgsize*sizeof(texel));
    assert(out!=NULL);

    // Allocate all the memory for the GPU
    texel *d_in, *d_out;
    cudaMalloc(&d_in, imgsize*sizeof(texel));
    cudaMalloc(&d_out, imgsize*sizeof(texel));

    // Calculate the number of streams we'll need
    streamCount = STREAMCOUNT;
    streamSize = ceil(((float)imgsize)/streamCount);
    blockCount = ceil(((float)streamSize)/threadCount);
    cudaStream_t* streams = (cudaStream_t*)malloc(streamCount*sizeof(cudaStream_t));
    printf("Creating kernel with %u streams containing %u blocks of %u threads each\n", streamCount, blockCount, threadCount); 

    // Get the current CPU time
    clock_t t = clock();

    // Apply the first shader pass (Vertical blur)
    int lastMemcpyOffset = 0;
    for (int i=0; i<streamCount; i++)
    {
        int offset = i*streamSize;
        int memcpySize = streamSize + imgw*(KERNELRADIUS-1)+(KERNELRADIUS-1);

        // Create the stream and launch the kernel
        cudaStreamCreate(&streams[i]);
        cudaMemcpyAsync(d_in+lastMemcpyOffset, img+lastMemcpyOffset, fmin(memcpySize, imgsize-lastMemcpyOffset)*sizeof(texel), cudaMemcpyHostToDevice, streams[i]);
        shader_pass1<<<blockCount, threadCount, 0, streams[i]>>>(d_out, d_in, offset, imgsize, imgw, imgh, filter);

        // Calculate the memory copy offsets
        lastMemcpyOffset += memcpySize;
    }

    // Apply the second shader pass (Horizontal blur + Desaturation)
    for (int i=0; i<streamCount; i++)
    {
        int offset = i*streamSize;

        // Launch the kernel and download the result
        shader_pass2<<<blockCount, threadCount, 0, streams[i]>>>(d_out, offset, imgsize, imgw, imgh, filter, saturation, 1-saturation);
        cudaMemcpyAsync(out+offset, d_out+offset, fmin(streamSize, imgsize-offset)*sizeof(texel), cudaMemcpyDeviceToHost, streams[i]);
    }
    cudaDeviceSynchronize();

    // Check if the kernel even executed
    cudaError_t err=cudaGetLastError();
    if (err!=cudaSuccess) {
        fprintf(stderr, "err=%u %s\n%s\n", (unsigned) err, cudaGetErrorString(err),
                "Problems executing kernel");
        exit(1);
    }

    // Calculate and print how long it took to execute 
    t = clock()-t;
    printf("time %f ms\n", t/(double)(CLOCKS_PER_SEC/1000));

    // Write the output to a new image file
    #if PRINT_PPM
        printImg(imgw, imgh, out);
    #endif
    f = fopen("out.ppm", "w");
    write_ppm(f, out, imgw, imgh, texelsize);
    fclose(f);

    // Exit the program
    return EXIT_SUCCESS;
}