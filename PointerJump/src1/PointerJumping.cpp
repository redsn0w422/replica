#include <sys/mman.h>
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

int *S;
int *W;

int *W_tmp;
int *S_tmp;    
int *W_rd, *W_wt;
int *S_rd, *S_wt;
static pthread_mutex_t *mutex;
int flag;

struct TEMP_ARGS{
  int pid;
};


void *single_thread(void *args){
  TEMP_ARGS * temp_args = (TEMP_ARGS*) args;
  int pid = temp_args->pid;

  int myStart = pid * (SIZE/NUM_THREADS);
  int myEnd = (pid + 1) * (SIZE/NUM_THREADS);
  if(pid==NUM_THREADS-1)
    myEnd = SIZE;

  for(int i = myStart; i < myEnd ; i++){
    if(S[i] != S[S[i]]){
      
      pthread_mutex_lock(mutex);
      flag += 1;
      pthread_mutex_unlock(mutex);
      
      W_wt[i] = W_rd[i] + W_rd[S_rd[i]];
      S_wt[i] = S_rd[S_rd[i]];
    }
    else{
      W_wt[i] = W_rd[i];
      S_wt[i] = S_rd[i];
    }

  }
  return NULL;
}


void pointer_jump(){
  W_rd = W;
  W_wt = W_tmp;
  S_rd = S;
  S_wt = S_tmp;
  int *tmp_ptr;

  flag=1;

  while(flag != 0){
    flag = 0;

    pthread_t* threads = new pthread_t[NUM_THREADS];
    TEMP_ARGS * arg = new TEMP_ARGS[NUM_THREADS];

    for( int i = 0; i < NUM_THREADS; i++ ) {
      arg[i].pid = i;
      pthread_create(threads+i,NULL, single_thread,(void*)&arg[i]);
    }

    for ( int i = 0; i < NUM_THREADS; i++) {
      pthread_join(threads[i],NULL);
    }

    delete[] threads;
    delete[] arg;

    tmp_ptr = W_rd;
    W_rd = W_wt;
    W_wt = tmp_ptr;
    tmp_ptr = S_rd;
    S_rd = S_wt;
    S_wt = tmp_ptr;
  }
}

int main(int argc, char **argv){

  S = (int *)malloc(sizeof(int)*SIZE);
  W = (int *)malloc(sizeof(int)*SIZE);
  W_tmp = (int *)malloc(sizeof(int)*SIZE);
  S_tmp = (int *)malloc(sizeof(int)*SIZE);

  int *ROI;
  // mutex = PTHREAD_MUTEX_INITIALIZER;
  mutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
  pthread_mutex_init(mutex, NULL);

  ROI = (int *)mmap((void *)0x30000000,sizeof(int),PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
  
  asm volatile("" ::: "memory");
  __sync_bool_compare_and_swap(ROI,0,0);
  asm volatile("" ::: "memory");

  for (int i = 0; i<SIZE; i++){
    S[i] = i+1;
    W[i] = 1;
  }

  S[SIZE-1] = SIZE-1;
  W[SIZE-1] = 0;

  pointer_jump();

  asm volatile("" ::: "memory");
  __sync_bool_compare_and_swap(ROI,0,0);
  asm volatile("" ::: "memory");
  
  // printf("Printing Results to file : %s \n", argv[1]);
  
  // std::ofstream outputFile(argv[1]);
  for (int i = 0; i<SIZE; i++){
    printf("%d ", W[i]);
  }
  printf("\n");
  // outputFile << "\n";
  return 0;
}
