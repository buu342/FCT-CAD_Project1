#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <time.h>
#include <ctype.h>
#include <string.h>
#include <math.h>


/*********************************
              Macros
*********************************/

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
typedef int color;
typedef struct {
    color r;
    color g;
    color b;
} texel;

// Blur matrix struct (workaround for passing arrays by value)
typedef struct {
  int data[KERNELSIZE][KERNELSIZE];
} blurMatrix;


/*==============================
    read_ppm
    Reads a PPM image ascii file
    @param The pointer to the image file handler 
    @param The pointer to an array of texels
    @param A pointer to store the image width 
    @param A pointer to store the image height
    @param A pointer to store the size of each texel 
==============================*/

void read_ppm(FILE* f, texel** img, size_t* width, size_t* height, size_t* texelsize)
{
    color r, g, b;
    uint32_t c;
    int count = 0;
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
            case 1: count += fscanf(f, "%lu%lu%lu", width, height, texelsize); break;
            case 2: count += fscanf(f, "%lu%lu", height, texelsize); break;
            case 3: count += fscanf(f, "%lu", texelsize);
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
    while (fscanf(f,"%d%d%d", &r, &g, &b) == 3)
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

void write_ppm(FILE* f, texel* img, const size_t width, const size_t height, const size_t texelsize)
{
    // Write the header
    fprintf(f, "P3\n%lu %lu %lu\n", width, height, texelsize);

    // Write the texel data
    for (uint32_t l=0; l<height; l++)
    {
        for (uint32_t c=0; c<width; c++) 
        {
            uint32_t p = (l*width + c);
            fprintf(f, "%d %d %d  ", img[p].r, img[p].g, img[p].b);
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

void printImg(const size_t imgw, const size_t imgh, const texel* img)
{
    for (uint32_t j=0; j<imgh; j++) 
    {
        for (uint32_t i=0; i<imgw; i++) 
        {
            uint32_t x = (i+j*imgw);
            printf("%d,%d,%d  ", img[x].r, img[x].g, img[x].b);
        }
        putchar('\n');
    }
    putchar('\n');
}


/*==============================
    areaFilter
    Performs a gaussian blur on a pixel of an image
    @param The image array to output to
    @param The image array to read from
    @param The texel line to modify
    @param The texel column to modify
    @param The image width (in texels)
    @param The image height (in texels)
    @param The gaussian blur matrix
==============================*/

void areaFilter(texel* out, texel* img, int line, int col, int width, int height, blurMatrix filter)
{
    color r = 0, g = 0, b = 0;
    int n = 0;

    // For each element in our gaussian matrix
    for (int l=line-(KERNELRADIUS-1); l<line+(KERNELSIZE-1) && l<height; l++)
        for (int c=col-(KERNELRADIUS-1); c<col+(KERNELSIZE-1) && c<width; c++)

            // If we don't go out of bounds
            if (l>=0 && c>=0) {
                size_t idx = (l*width+c);
                int scale = filter.data[l-line+(KERNELRADIUS-1)][c-col+(KERNELRADIUS-1)];
                texel* to = &img[idx];

                // Accumulate the texel colors from neighbors
                r += scale*to->r;
                g += scale*to->g;
                b += scale*to->b;
                n += scale;
            }

    // Normalize the color
    size_t idx = (line*width+col);
    texel* ti = &out[idx];
    ti->r=r/n;
    ti->g=g/n;
    ti->b=b/n;
}


/*==============================
    pointFilter
    Desaturates a pixel on an image
    @param The image array to output to
    @param The image array to read from
    @param The texel line to modify
    @param The texel column to modify
    @param The image width (in texels)
    @param The image height (in texels)
    @param The amount of saturation (ranging from 0 to 1).
==============================*/

void pointFilter(texel *out, texel *img, size_t line, size_t col, size_t width, size_t height, float saturation) 
{
    color r = 0, g = 0, b = 0;
    float grey;
    size_t idx = (line*width+col);
    texel* to = &img[idx];
    texel* ti = &out[idx];

    // Get the texel data
    r = to->r;
    g = to->g;
    b = to->b;

    // Calculate the desaturated texel
    grey = saturation * (0.3 * r + 0.59 * g + 0.11 * b);
    ti->r=(1-saturation)*r+grey;
    ti->g=(1-saturation)*g+grey;
    ti->b=(1-saturation)*b+grey;
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
    size_t imgh, imgw, imgsize;
    size_t texelsize;
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
    printf("PPM image %lux%lu. Texel size = %lu\n", imgw, imgh, texelsize);
    imgsize = imgw*imgh;
    #if PRINT_PPM
        printImg(imgw, imgh, img);
    #endif
    fclose(f);

    // Calculate the gaussian blur matrix
    for (size_t x=0; x<KERNELSIZE; x++)
        for (size_t y=0; y<KERNELSIZE; y++)
            filter.data[x][y] = gaussian(x, y)*100;

    // Allocate memory for the outputted image data buffer
    texel *out = (texel*)malloc(imgsize*sizeof(texel));
    assert(out!=NULL);

    // Get the current CPU time
    clock_t t = clock();

    // Gaussian blur
    for (size_t l=0; l<imgh; l++) {
        for (size_t c=0; c<imgw; c++) {
            areaFilter(out, img, l, c, imgw, imgh, filter);
        }
    }

    // Color correction
    for (size_t l=0; l<imgh; l++) {
        for (size_t c=0; c<imgw; c++) {
            pointFilter(out, out, l, c, imgw, imgh, saturation);
        }
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