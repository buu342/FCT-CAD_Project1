# Define compiler flags based on debug mode being enabled or not
ifeq ($(DEBUG), 1)
	CFLAGS = -g
else
	CFLAGS = -O3
endif

# Compile the program
all: main.cu
	nvcc $(CFLAGS) -o program $< -lm

# Clean up
.PHONY:	clean
clean:
	rm -f program out.ppm