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
#include <time.h>
#include <pthread.h>
#include "wireless.h"

#ifndef SIZE
#define SIZE 100
#endif

#ifndef NUM_THREADS
#define NUM_THREADS 4
#endif

FILE * logfile;

typedef struct timespec stime_t;

#ifndef INLINE
#define INLINE static inline
#endif

#ifndef CONST
#ifdef __cplusplus
#define CONST const
#else
#define CONST /*empty*/
#endif
#endif

INLINE void gettime(stime_t * pTimeDest) {
  #ifdef MEASURE_TIME
  // POSSIBLE CLOCKS:
  //   CLOCK_REALTIME
  //          System-wide  realtime clock.  Setting this clock requires appro-
  //          priate privileges.
  //   CLOCK_MONOTONIC
  //          Clock that cannot be set and  represents  monotonic  time  since
  //          some unspecified starting point.
  //   CLOCK_PROCESS_CPUTIME_ID
  //          High-resolution per-process timer from the CPU.
  //   CLOCK_THREAD_CPUTIME_ID
  //          Thread-specific CPU-time clock.
  clock_gettime(CLOCK_MONOTONIC, pTimeDest);
  //gettimeofday(pTimeDest,0);
  #endif
}

INLINE void gettime2(stime_t * pTimeDest, int timerMode) {
  #ifdef MEASURE_TIME
  clock_gettime(timerMode, pTimeDest);
  #endif
}

#define NSEC_SCALE 1000000000
/// Returns the interval between two moments
INLINE void delta_t(stime_t *interval, CONST stime_t *begin, CONST stime_t *now) {
  #ifdef MEASURE_TIME
  interval->tv_nsec = now->tv_nsec - begin->tv_nsec;
  if(interval->tv_nsec < 0 ){
    interval->tv_nsec += NSEC_SCALE;
    interval->tv_sec = now->tv_sec - begin->tv_sec - 1;
  }
  else{
    interval->tv_sec = now->tv_sec - begin->tv_sec;
  }
  #endif
}

/// Assigns the value of one time variable to the other
INLINE void settime(stime_t * pTimeDest, CONST stime_t * pTimeSrc) {
  #ifdef MEASURE_TIME
  pTimeDest->tv_sec = pTimeSrc->tv_sec;
  pTimeDest->tv_nsec = pTimeSrc->tv_nsec;
  #endif
}

/// Prints time to the string constant (whole numbers are seconds, fractions up to nanoseconds)
INLINE char * printTime(CONST stime_t * t, char * printbuffer) {
  sprintf(printbuffer, "%ld.%09ld", t->tv_sec, t->tv_nsec);
  return printbuffer;
}

using namespace std;

int *A;
int *B;
int *C;
int *LocalSums;

int *W_tmp;
int *S_tmp;    
int *W_rd, *W_wt;
int *S_rd, *S_wt;
static pthread_mutex_t mutex;
pthread_barrier_t *  barrier; 

struct TEMP_ARGS{
  int pid;
};

stime_t start,end, delta;

void *single_thread(void *args){
  TEMP_ARGS * temp_args = (TEMP_ARGS*) args;
  int pid = temp_args->pid;

  stime_t l_start, l_end, l_diff;
  char l_buff[20];
  // printf("here TID : %d\n", pid);

  int myStart = pid * (SIZE/NUM_THREADS);
  int myEnd = myStart + (SIZE/NUM_THREADS);

  if(pid==NUM_THREADS-1)
    myEnd = SIZE;

  // cout << pid << "=>" << myStart << " - " << myEnd << endl;
  // LocalSums[pid] = 0;
  int stride = 1;
  int j=0,temp1,temp2;

  while (stride<=SIZE){
    // printf("Stride %d\n", stride);
    gettime(&l_start);
    for(int i = myStart; i < myEnd ; i++){
        B[i] = A[i];
      
    }
    gettime(&l_end);
    delta_t(&l_diff, &l_start, &l_end);
    printTime(&l_diff, l_buff);
    printf("TID : %d : TIME DIFF : %s\n",pid,l_buff);
    //printf("At the barrier : %d\n", pid);

    gettime(&l_start);
    pthread_barrier_wait (barrier);

    gettime(&l_end);
    delta_t(&l_diff, &l_start, &l_end);
    printTime(&l_diff, l_buff);
    printf("Barrier Iter : %d : TIME DIFF : %s\n", j, l_buff);

     //    printf("Past the barrier\n");
  
    for(int i = myStart; i < myEnd ; i++){
      if (i>=stride){

        gettime(&l_start);
        temp2 = A[i];
        temp1 = B[i-stride];
        gettime(&l_end);
        delta_t(&l_diff, &l_start, &l_end);
        printTime(&l_diff, l_buff);
        printf("T2 Iter : %d : TIME DIFF : %s\n", j, l_buff);

        gettime(&l_start);
        A[i] = temp1 + temp2;
        gettime(&l_end);
        delta_t(&l_diff, &l_start, &l_end);
        printTime(&l_diff, l_buff);
        printf("T3 Iter : %d : TIME DIFF : %s\n", j, l_buff);
      }
    }

    gettime(&l_start);
    pthread_barrier_wait (barrier);
    
    gettime(&l_end);
    delta_t(&l_diff, &l_start, &l_end);
    printTime(&l_diff, l_buff);
    fprintf(logfile, "Barrier2 Iter : %d : TIME DIFF : %s\n", j, l_buff);
    j++;
    stride*=2;
  }

  return NULL;
}

int main(int argc, char **argv){
  logfile = fopen("results.txt","w");
  barrier = (pthread_barrier_t*)wireless_alloc(sizeof(pthread_barrier_t));
  pthread_barrier_init (barrier, NULL, NUM_THREADS);
  
  A = (int *)malloc(sizeof(int)*SIZE);
  B = (int *)malloc(sizeof(int)*SIZE);
  // C = (int *)malloc(sizeof(int)*SIZE*log(SIZE));
  // S_tmp = (int *)malloc(sizeof(int)*SIZE);

  gettime(&start);
  char buff[20];
  printTime(&start, buff);
  cout << "TIME START : " << buff << endl;
  

  int *ROI;
  //mutex = PTHREAD_MUTEX_INITIALIZER;

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

  gettime(&end);
  delta_t(&delta, &start, &end);
  printTime(&delta, buff);
  // cout << "TIME DIFF : " << buff << endl;
  
  // for (int i = 0; i<SIZE; i++){
  //   printf("%d, ", A[i]);
  // }
  // printf("\n");

  return 0;
}
