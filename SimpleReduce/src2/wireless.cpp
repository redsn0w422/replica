#include <sys/mman.h>
#include <iostream>
#include <stdio.h>
#include <unistd.h>

#ifndef BARRIER_START
#define BARRIER_START 32000000
#endif

#ifndef BARRIER_SIZE
#define BARRIER_SIZE 4096
#endif

#ifndef OTHER_START
#define OTHER_START 31000000
#endif

#ifndef OTHER_SIZE
#define OTHER_SIZE 16384
#endif

unsigned int current_loc_barrier = 0;
unsigned int current_loc = 0x31000000;
unsigned int limit = 0x31000000;
long pageNumber = 0;

void* wireless_barrier_alloc(unsigned int size){
  if(current_loc_barrier==0){
    current_loc_barrier += size;
    return mmap((void *)0x32000000, 4096, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
  }
  int temp = current_loc_barrier;
  current_loc_barrier += size;
  return (void *)(0x32000000 + temp);
}

void* wireless_alloc(unsigned int size){
  fprintf(stderr, "Size : %d \nLimit : 0x%x\n", size, current_loc + size);
  if((current_loc + size) > limit){
    while(current_loc + size > limit){
      void * temp =  mmap((void *)(0x31000000 + 4096 * pageNumber), 4096, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
      limit += 4096;
      pageNumber++;
      //fprintf(stderr, "Allocating Memory. Page %ld\n", pageNumber);
    }
    long temp_addr = current_loc;
    current_loc += size;
    //fprintf(stderr, "Current wireless memory location : 0x%x\n", current_loc);
    return (void *)temp_addr;
  }
  long temp_addr = current_loc;
  current_loc += size;
  //  std::cout << current_loc << "\n";
  //fprintf(stderr, "Current wireless memory location : 0x%x\n", current_loc);
  return (void *)temp_addr;
}

void* initializeROI(){
  return mmap((void *)0x30000000,sizeof(int),PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
}
