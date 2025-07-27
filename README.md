# Multithreaded Memory Allocator
This program implements a multithreaded memory allocator that handles small and large blocks. It is thread safe, and utilizes 3 different modes to choose when dealing with concurrency: sequential, coarse-grain, and fine-grain. I was given starter code that only handled small blocks sequentially. I was assigned to expand this to handle small and larger blocks, as well as utilize coarse-grain and fine-grain concurrency. Coarse grain concurrency implements a lock whenever more than one thread attempts to free or malloc data at the same time. Fine-grain concurrency is implemented by giving each thread its own list of memory. When a thread's memory runs out, it overflows into a separate 'Overflow' list. Thus, when freeing data in this mode, it is crucial to determine where the original memory block lies by looking at the memory address, so it can determine if it came from the original thread list or the overflow list. Each list of memory in this project utilizes the first half of the memory for small blocks, and the second half for large blocks.

# Directions
- create a file driver.c (a sample file was provided) and call the functions to handle the malloc functions defined in myMalloc.h
- in driver.c, myInit(numThreads, flag) must be called before any calls to myMalloc() or myFree(). numThreads is the number of threads to run the program, and flag is an int representing what mode to use to handle concurrency. A flag of 0 is sequential, 1 is used for coarse-grain, and 2 is used for fine-grain. A malloc request should never exceed 1024.
- cd into the project directory in bash and run 'make' to create the 'driver' executable.
- run 'driver' to execute the commands defined in driver.c.

# Credit
The sample files, myMalloc.h, myMalloc.c, driver.c, myMalloc-helper.c, myMalloc-helper.h, and Makefile were supplied by my professor, David Lowenthal. I expanded myMalloc.c to handle concurrency in coarse-grain and fine-grain modes, as well as handling large blocks of data.
