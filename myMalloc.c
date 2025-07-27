// CSc 422
// Program 2 code for sequential myMalloc (small blocks *only*)
// You must add support for large blocks and then concurrency (coarse- and fine-grain)

#include <stdlib.h>
#include <stdio.h>
#include "myMalloc-helper.h"
#include <pthread.h>

// total amount of memory to allocate, and size of each small chunk
#define SIZE_TOTAL 276672
#define SIZE_SMALL 64
#define SIZE_LARGE 1024

// maintain lists of free blocks and allocated blocks
typedef struct memoryManager {
  chunk *freeSmall;
  chunk *allocSmall;
  chunk *freeLarge;
  chunk *allocLarge;
  int memLeftSmall;
  int memLeftLarge;

  void *mem;  //start of memory location
} memManager;


// The list used for overflow
memManager *overflow;


// mutex
pthread_mutex_t mutexOverflow;
pthread_mutex_t idLock; 

pthread_key_t key;

// Id array to identify 
int IDarray[8] = {1,2,3,4,5,6,7,8};
int curKey = 0;

// For touching files only once
int seenId[9] = {0,0,0,0,0,0,0,0,0};

// the memory managers for each thread
memManager *mMans[8] = {NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};




// note that flag is not used here because this is only the sequential version
int myInit(int numCores, int flag) {
  int numSmall;
  int numLarge;

  // linked list of thread memories

  // Set size and initialize locks and keys
  int sizeMem;
  if (flag == 0){
    sizeMem = 0;  //memory size of zero so overflow gauranteed.
  }
  else if (flag == 1){
    pthread_mutex_init(&idLock,NULL);
    pthread_mutex_init(&mutexOverflow,NULL);
    sizeMem = 0;  //memory size of zero so overflow gauranteed.
  }
  else{
    pthread_mutex_init(&idLock,NULL);
    pthread_mutex_init(&mutexOverflow,NULL);
    sizeMem = SIZE_TOTAL; //memory size of SIZE_TOTAL for each memory
  }
  

  // how many small chunks are there?
  // note that we divide by the *sum* of 64 bytes (for what user gets) and our metadata.
  // the division by 2 is because when you extend to handle large blocks, the small blocks
  // are to take up only half of the total.
  numSmall = (SIZE_TOTAL/2) / (SIZE_SMALL + sizeof(chunk));
  numLarge = (SIZE_TOTAL/2) / (SIZE_LARGE + sizeof(chunk));


  // For each thread, initialize memory lists to zero if flag == 1, SIZE_TOTAL otherwise
  for (int i = 0; i < numCores; i++){

    // Setup each mManager per threadMemory node
    memManager *new = (memManager *) malloc(sizeof(memManager));
    mMans[i] = new;

    if (sizeMem == 0){
      new->mem = NULL;
    }
    else{
      new->mem = malloc(SIZE_TOTAL);
    }

    new->allocLarge = createList();
    new->allocSmall = createList();
    new->freeLarge =  createList();
    new->freeSmall =  createList();
    new->memLeftLarge = sizeMem/2;
    new->memLeftSmall = sizeMem/2;



    // If the size of each memory manager is zero, do nothing. Otherwise, set up chunks
    if (sizeMem != 0){
      // set up free list chunks
      setUpChunks(new->freeSmall, new->mem, numSmall, SIZE_SMALL);
      setUpChunks(new->freeLarge, (char *)(new->mem) + SIZE_TOTAL / 2, numLarge, SIZE_LARGE);
    }


  }

  // Create overflow List
  overflow = (memManager *) malloc(sizeof(memManager));
  overflow->allocLarge = createList();
  overflow->allocSmall = createList();
  overflow->freeLarge = createList();
  overflow->freeSmall = createList();
  overflow->memLeftLarge = SIZE_TOTAL/2;
  overflow->memLeftSmall = SIZE_TOTAL/2;
  overflow->mem = malloc(SIZE_TOTAL);
  

  setUpChunks(overflow->freeSmall, overflow->mem, numSmall, SIZE_SMALL);
  setUpChunks(overflow->freeLarge, (char *)(overflow->mem) + SIZE_TOTAL / 2, numLarge, SIZE_LARGE);


  pthread_key_create(&key, NULL);

  return 0;
}




// Applies setspecific to a thread to identify by id (only first time)
void addThreadId(){

  if (pthread_getspecific(key)==NULL){
    pthread_mutex_lock(&idLock);
    if (pthread_getspecific(key)==NULL){
      pthread_setspecific(key, (void *)&IDarray[curKey]);
      curKey++;
    }
    pthread_mutex_unlock(&idLock);
  }

}



// Helper to select mem by thread id 1-8
memManager *selectMemById(int id){
  return mMans[id-1];
}

// Uses pthread_getspecific to get correlating memManager for thread
memManager *getTargetMemory(){
  memManager *target;

  int id = *((int *)pthread_getspecific(key));
  target = selectMemById(id);

  return target;
}

// Is the ptr from overflow (not of current memManager?)
int pointerFromOverflow(void *ptr, memManager *mMan) {

  if (mMan -> mem == NULL){
    return 1;
  }

  return (ptr < mMan->mem) || (ptr >= (void *)((char *)mMan->mem + SIZE_TOTAL));
}

// No ids seen yet in seenId
int nothingSeen(){
  for (int i = 0; i < 9; i++){
    if (seenId[i] == 1){
      return 0;
    }
  }

  return 1;
}

// myMalloc just needs to get the next chunk and return a pointer to its data 
// note the pointer arithmetic that makes sure to skip over our metadata and
// return the user a pointer to the data
void *myMalloc(int size) {
  
  addThreadId();
  int id = *((int *)pthread_getspecific(key));

  memManager *target = getTargetMemory();
  int overflowed = 0;

  chunk *toAlloc;
  int allocSize;

  

  // alloc Small chunk
  if (size <= SIZE_SMALL){
    allocSize = (SIZE_SMALL + sizeof(chunk));

    // Check to overflow
    if(target->memLeftSmall < allocSize){
      target = overflow;
      overflowed = 1;
      pthread_mutex_lock(&mutexOverflow);
    }

    toAlloc = getChunk(target->freeSmall, target->allocSmall);   

    target->memLeftSmall -= allocSize;
  }



  // Alloc large chunk
  else{
    allocSize = (SIZE_LARGE + sizeof(chunk));

    // Check to overflow
    if(target->memLeftLarge < allocSize){
      target = overflow;
      overflowed = 1;
      pthread_mutex_lock(&mutexOverflow);
    }

    toAlloc = getChunk(target->freeLarge, target->allocLarge); 

    target->memLeftLarge -= allocSize;
  }

  
  // TOUCH FILES
  if(overflowed){
    if (nothingSeen()){   // We know that program is sequential if overflow happens with no threads seen yet
      char str[24];
      sprintf(str, "touch Id-1\n");
      system(str);
      seenId[0] = 1;
      seenId[8] = 1;
    }
    else if (!seenId[8]) {  // If overflow is not yet marked, touch Overflow
      char str[24];
      sprintf(str, "touch Overflow\n");
      system(str);
      seenId[8] = 1;
    }
  }
  else if (!seenId[id-1]) { // If overflow hasn't happened, mark corresponding thread as seen and touch
      char str[24];
      sprintf(str, "touch Id-%d\n", id);
      system(str);
      seenId[id-1] = 1;
  }



  void *toReturn = ((void *) ((char *) toAlloc) + sizeof(chunk));
  
  if (overflowed) pthread_mutex_unlock(&mutexOverflow);
  
  return toReturn;
}




// myFree just needs to put the block back on the free list
// note that this involves taking the pointer that is passed in by the user and
// getting the pointer to the beginning of the chunk (so moving backwards chunk bytes)
void myFree(void *ptr) {
  memManager *target = getTargetMemory();
  int fromOverflow = pointerFromOverflow(ptr, target);

  // lock if getting from overflow list
  if (fromOverflow){
    pthread_mutex_lock(&mutexOverflow);
    target = overflow;
  }


  // find the front of the chunk
  chunk *toFree = (chunk *) ((char *) ptr - sizeof(chunk));

  int sizeFreed = sizeof(chunk) + toFree->allocSize;


  // Return small chunk
  if (toFree->allocSize == SIZE_SMALL){
    returnChunk(target->freeSmall, target->allocSmall, toFree);
    target->memLeftSmall += sizeFreed;
  }

  // Return large chunk
  else{
    returnChunk(target->freeLarge, target->allocLarge, toFree);
    target->memLeftLarge += sizeFreed;

  }


  if(fromOverflow) pthread_mutex_unlock(&mutexOverflow);
  
}

