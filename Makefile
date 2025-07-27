driver:	driver.o myMalloc.o myMalloc-helper.o
	gcc -o driver driver.o myMalloc.o myMalloc-helper.o -lpthread

driver.o:	testFine.c myMalloc.h
	gcc -g -c testFine.c -o driver.o

myMalloc.o:	myMalloc.c myMalloc.h myMalloc-helper.h
	gcc -g -c myMalloc.c

myMalloc-helper.o:	myMalloc-helper.c myMalloc-helper.h
	gcc -g -c myMalloc-helper.c

clean:
	rm -f *.o driver
