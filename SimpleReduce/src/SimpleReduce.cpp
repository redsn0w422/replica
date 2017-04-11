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

#ifndef SIZE
#define SIZE 100
#endif

#ifndef NUM_THREADS
#define NUM_THREADS 4
#endif

using namespace std;

int *Numbers;
int *LocalSums;

int *W_tmp;
int *S_tmp;    
int *W_rd, *W_wt;
int *S_rd, *S_wt;
static pthread_mutex_t mutex;
int flag;

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

  //cout << pid << "=>" << myStart << " - " << myEnd << endl;
  LocalSums[pid] = 0;
  
  for(int i = myStart; i < myEnd ; i++){
    LocalSums[pid] += Numbers[i];
  }
  return NULL;
}

int main(int argc, char **argv){

  Numbers = (int *)malloc(sizeof(int)*SIZE);
  LocalSums = (int *)malloc(sizeof(int)*NUM_THREADS);
  W_tmp = (int *)malloc(sizeof(int)*SIZE);
  S_tmp = (int *)malloc(sizeof(int)*SIZE);

  int *ROI;
  //mutex = PTHREAD_MUTEX_INITIALIZER;

  ROI = (int *)mmap((void *)0x30000000,sizeof(int),PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
  asm volatile("" ::: "memory");
  __sync_bool_compare_and_swap(ROI,0,0);
  asm volatile("" ::: "memory");

  for (int i = 0; i<SIZE; i++){
    Numbers[i] = i;
  }

  pthread_t* threads = new pthread_t[NUM_THREADS];
  TEMP_ARGS * arg = new TEMP_ARGS[NUM_THREADS];
  for( int i = 0; i < NUM_THREADS; i++ ) {
     
    arg[i].pid = i;

    pthread_create(threads+i,NULL, single_thread,(void*)&arg[i]);

  }


  for ( int i = 0; i < NUM_THREADS; i++) {
    pthread_join(threads[i],NULL);
  }

  int sum=0;
  for ( int i = 0; i < NUM_THREADS; i++) {
    sum += LocalSums[i];
  }

  asm volatile("" ::: "memory");
  __sync_bool_compare_and_swap(ROI,0,0);
  asm volatile("" ::: "memory");
  
  printf("Sum %d\n", sum);
  
  // std::ofstream outputFile(argv[1]);
  // for (int i = 0; i<SIZE; i++){
  //   outputFile << W[i] << "\n";
  // }

  return 0;
}
