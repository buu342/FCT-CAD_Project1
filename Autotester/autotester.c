/***************************************************************
                          autotester.c

Automated testing tool for Energy Storms implementations.
https://github.com/buu342/cp2020-2021_g41_45511_54449_54530/
***************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>


/*********************************
              Macros
*********************************/

// Program and folder names
#define PROGRAM_NAME    "program"
#define PROGRAMS_FOLDER "./../Source Code/"
#define RESULTS_FOLDER  "./../Results/"
#define IMAGES_FOLDER  "./../Images/"

// Maximum supported file path size. 1KB should be enough...
#define MAXPATH 1024

// Color definitions
#define CR_RED    "\033[0;31m"
#define CR_GREEN  "\033[0;32m"
#define CR_YELLOW "\033[0;33m"
#define CR_BLUE   "\033[0;34m"
#define CR_RESET  "\033[0m"


/*==============================
    terminate
    Stops the program, printing a message before doing so.
    @param A string to print, or NULL
==============================*/

void terminate(const char* endmsg)
{
	if (endmsg != NULL)
		printf(CR_RED"%s"CR_RESET, endmsg);
	exit(0);
}


/*==============================
    main
    Program entrypoint
    @param The number of arguments passed to the program
    @param Array of the program arguments, as strings
==============================*/

int main(int argc, char* argv[])
{
	int ret;
	int repeats = 0;
	int size;
	DIR *dir;
	char image[MAXPATH/2];
	struct dirent *entry;

	// Ensure we have enough arguments
	if (argc < 3)
	{
		terminate(CR_RESET"Program Usage: ./autotester <image> <iterations>]\n");
		return 0;
	}

	// Get our arguments
	repeats = atoi(argv[2]);
	strcpy(image, argv[1]);

	// Check if we have an images folder
	if ((dir = opendir(IMAGES_FOLDER)) == NULL)
		terminate("Error: Missing 'Images' directory!");

	// Check if we have a results folder
	if ((dir = opendir(RESULTS_FOLDER)) == NULL)
		terminate("Error: Missing 'Results' directory!");

	// Check if we have a folder with all our programs
	if ((dir = opendir(PROGRAMS_FOLDER)) == NULL)
		terminate("Error: Missing 'Source Code' directory!");

	// Clear the results folder
	printf(CR_BLUE"Clearing Results folder\n"CR_RESET);
	ret = system("rm -f -r "RESULTS_FOLDER"*");

	// Iterate through all folders
	printf(CR_BLUE"Starting tests\n"CR_RESET);
	while ((entry = readdir(dir)) != NULL)
	{
		char path[MAXPATH];
		const char* fname = entry->d_name;

		// Filter out unwanted files
		if (!strcmp(fname, ".") || !strcmp(fname, "..") || strstr(fname, ".sh"))
			continue;

		// Check the program exists
		sprintf(path, PROGRAMS_FOLDER"%s/"PROGRAM_NAME, fname);
		if (access(path, F_OK))
			terminate("Error: Could not find '"PROGRAM_NAME"'. Did you remember to compile them all?\n");

		// Make the results directory
		sprintf(path, "mkdir \""RESULTS_FOLDER"%s/\"", fname);
		ret = system(path);

		// Run the program a bunch of times
		printf(CR_YELLOW"\nTesting '%s'"CR_RESET"\n", fname);
		for (int i=0; i<repeats; i++)
		{
			// Execute the program
			sprintf(path, "nvprof \""PROGRAMS_FOLDER"%s/"PROGRAM_NAME"\" \""IMAGES_FOLDER"%s\" > \""RESULTS_FOLDER"%s/results_%02d.txt\"", fname, image, fname, i+1);
			ret = system(path);
		}
	}

	// Iterate through all folders
	printf(CR_BLUE"\nCalculating average times\n"CR_RESET);
	dir = opendir(RESULTS_FOLDER);
	while ((entry = readdir(dir)) != NULL)
	{
		FILE* fp;
		char path[MAXPATH];
		const char* fname = entry->d_name;
		double time = 0;
		double algtime = 0;

		// Filter out unwanted files
		if (!strcmp(fname, ".") || !strcmp(fname, ".."))
			continue;

		// Get each tests' time
		for (int i=0; i<repeats; i++)
		{
			char * substr;
			char line[MAXPATH];

			// Open the results file
			sprintf(path, RESULTS_FOLDER"%s/results_%02d.txt", fname, i+1);
			fp = fopen(path, "r+"); 

			// Find the time substring
			while (fgets(line, MAXPATH, fp) != NULL)
				if (line[0] == 't' && line[1] == 'i' && line[2] == 'm' && line[3] == 'e')
					break;

			// Now get the time
			substr = strtok(line, " ");  // "time"
			substr = strtok (NULL, " "); // The time in ms
			time += atof(substr);

			// Continue reading the string
			substr = strtok (NULL, " "); // "ms"
			substr = strtok (NULL, " "); // EOL? Or algorithm 

			// Check for average time
			if (substr != NULL)
			{
				substr = strtok (NULL, " "); // "(average"
				substr = strtok (NULL, " "); // The average time in ms
				algtime += atof(substr);
			}
			fclose(fp);
		}

		// Print the times
		printf(CR_GREEN"%s"CR_RESET" - %f ms", fname, time/repeats);
		if (algtime != 0)
			printf("(%f ms)", algtime/repeats);
		printf("\n");
	}

	// End
	terminate(CR_BLUE"\nFinished!\n");
	return 0;
}