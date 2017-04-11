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
static pthread_mutex_t mutex;
int flag;

int *MYROI;

struct TEMP_ARGS{
  int pid;
};


void *single_thread(void *args){
  TEMP_ARGS * temp_args = (TEMP_ARGS*) args;
  int pid = temp_args->pid;

  int myStart = pid * (SIZE/NUM_THREADS);
  int myEnd = (pid + 1) * (SIZE/NUM_THREADS);
  // printf("Thread %d : %d-%d\n", pid, myStart, myEnd);

  int i;
  for(i = myStart; i < myEnd ; i++){
    if(S[i] != S[S[i]]){
      pthread_mutex_lock(&mutex);
      flag++;
      pthread_mutex_unlock(&mutex);
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
  int j =0;
  while(flag != 0){
    flag = 0;
    j++;
    
    asm volatile("" ::: "memory");
    __sync_bool_compare_and_swap(MYROI,0,0);
    asm volatile("" ::: "memory");

    pthread_t* threads = new pthread_t[NUM_THREADS];
    TEMP_ARGS * arg = new TEMP_ARGS[NUM_THREADS];
    for( int i = 0; i < NUM_THREADS; i++ ) {
      arg[i].pid = i;
      pthread_create(threads+i,NULL, single_thread,(void*)&arg[i]);
    }

    for ( int i = 0; i < NUM_THREADS; i++) {
      pthread_join(threads[i],NULL);
    }

    printf("Iteration : %d\n", j);
    printf("Flag : %d\n", flag);
    printf("W_rd :");
    for (int i=0; i<SIZE; i++)
      printf("%d ", W_rd[i]);
    printf("\n");
    printf("S_rd :");
    for (int i=0; i<SIZE; i++)
      printf("%d ", S_rd[i]);
    
    printf("\nW_wt :");
    for (int i=0; i<SIZE; i++)
      printf("%d ", W_wt[i]);
    printf("\n");
    printf("S_wt :");
    for (int i=0; i<SIZE; i++)
      printf("%d ", S_wt[i]);
    printf("\n");
    
    delete[] threads;
    delete[] arg;

    tmp_ptr = W_rd;
    W_rd = W_wt;
    W_wt = tmp_ptr;
    tmp_ptr = S_rd;
    S_rd = S_wt;
    S_wt = tmp_ptr;

    asm volatile("" ::: "memory");
    __sync_bool_compare_and_swap(MYROI,0,0);
    asm volatile("" ::: "memory");
  }
}

int main(int argc, char **argv){

  S = (int *)wireless_alloc(sizeof(int)*SIZE);
  W = (int *)wireless_alloc(sizeof(int)*SIZE);
  W_tmp = (int *)wireless_alloc(sizeof(int)*SIZE);
  S_tmp = (int *)wireless_alloc(sizeof(int)*SIZE);
  //  flag = (int *)wireless_alloc(sizeof(int));
  flag = 1;
  
  int *ROI;

  printf("Memory Allocation S : 0x%x, W : 0x%x, W_temp : 0x%x, S_tmp : 0x%x\n", S, W, W_tmp, S_tmp);
  
  ROI = (int *)mmap((void *)0x30000000,sizeof(int),PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
  MYROI = (int *)mmap((void *)0x40000000,sizeof(int),PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
  
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
  
  printf("Printing Results\n");
  
  for (int i = 0; i<SIZE; i++){
    cout << W[i] << " ";
  }
  cout << endl;
  
  return 0;
}
