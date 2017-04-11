#include <sys/mman.h>
#include <math.h>
#include <pthread.h>
#include <cxxabi.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <cstdlib>
#include <memory>
#include <map>
#include <list>
#include <vector>
#include <fstream>
#include <pthread.h>
#include "wireless.h"

#ifndef SIZE
#define SIZE 100
#endif

#ifndef NUM_THREADS
#define NUM_THREADS 4
#endif

using namespace std;

int *A;
int *B;

int* A_rd, *A_wt;

int *temp;

static pthread_mutex_t mutex;
pthread_barrier_t *  barrier; 

struct TEMP_ARGS{
  int pid;
};


void *single_thread(void *args){
  TEMP_ARGS * temp_args = (TEMP_ARGS*) args;
  int pid = temp_args->pid;

  int myStart = pid * (SIZE/NUM_THREADS);
  int myEnd = myStart + (SIZE/NUM_THREADS);

  if(pid==NUM_THREADS-1)
    myEnd = SIZE;

  int stride = 1;

  while (stride<=SIZE){
  
    for(int i = myStart; i < myEnd ; i++){
      if (i>=stride){
        A_wt[i] = A_rd[i] + A_rd[i-stride];
      }
      else{
        A_wt[i] = A_rd[i];
      }
    }

    pthread_barrier_wait (barrier);
    if(pid==0){
      temp = A_rd;
      A_rd = A_wt;
      A_wt = temp;
    }
    pthread_barrier_wait (barrier);
    stride*=2;
  }

  return NULL;
}


int main(int argc, char **argv){

  barrier = (pthread_barrier_t*)wireless_alloc(sizeof(pthread_barrier_t));
  pthread_barrier_init (barrier, NULL, NUM_THREADS);
  
  A = (int *)malloc(sizeof(int)*SIZE);
  B = (int *)malloc(sizeof(int)*SIZE);

  A_rd = A;
  A_wt = B;

  int *ROI;

  ROI = (int *)mmap((void *)0x30000000,sizeof(int),PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
  asm volatile("" ::: "memory");
  __sync_bool_compare_and_swap(ROI,0,0);
  asm volatile("" ::: "memory");

  for (int i = 0; i<SIZE; i++){
    A[i] = i;
  }

  printf("Starting the threads\n");
  int err;
  pthread_t* threads = new pthread_t[NUM_THREADS];
  TEMP_ARGS * arg = new TEMP_ARGS[NUM_THREADS];
  for( int i = 0; i < NUM_THREADS; i++ ) {
     
    arg[i].pid = i;

    err = pthread_create(&threads[i],NULL, single_thread,(void*)&arg[i]);

  }


  for ( int i = 0; i < NUM_THREADS; i++) {
    pthread_join(threads[i],NULL);
  }

  asm volatile("" ::: "memory");
  __sync_bool_compare_and_swap(ROI,0,0);
  asm volatile("" ::: "memory");
  
  for (int i = 0; i<SIZE; i++){
    printf("%d, ", A_rd[i]);
  }
  printf("\n");

  return 0;
}
