#include <libmemcached/memcached.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define SPREAD_GET_RANGE 100
#define SPREAD_WRITE_RANGE 100
#define MAX_THREADS 50
#define TIMER_SPAWN_THREAD 1

long size=10000;
long thread_counter=0;
char server[100];
int port=11211, package_size=1024, spread_range=30;

pthread_mutex_t mutex_stdio = PTHREAD_MUTEX_INITIALIZER;

int fill_value(int spread_range){
  memcached_server_st *servers = NULL;
  memcached_st *memc;
  memcached_return rc;

  char value[1000000];  //The default maximum object size is 1MB in memcached
  char key[20];
  int i=0;
  
  memc = memcached_create(NULL);

  servers = memcached_server_list_append(servers, server, port, &rc);
  rc = memcached_server_push(memc, servers);
  
  if (rc != MEMCACHED_SUCCESS){
    fprintf(stderr, "Couldn't add server for write: %s\n", memcached_strerror(memc, rc));
    exit(1);
  }

  for( ; spread_range>=0; spread_range--){
    for (i=0; i < size; i++){
      value[i]='A';
    }

    value[i]='\0';
    sprintf(key,"test%d",spread_range);
    rc = memcached_set(memc, key, strlen(key), value, strlen(value), (time_t)0, MEMCACHED_BEHAVIOR_TCP_NODELAY);

    if (rc != MEMCACHED_SUCCESS){
      fprintf(stderr, "Couldn't store key: %s\n", memcached_strerror(memc, rc));
      exit(1);
    }
  }

  return 0;
  
}


double get_wall_time() {
  int i;
  struct timespec t;
  if((i = clock_gettime(CLOCK_MONOTONIC_RAW, &t))) {
    printf("\nError: %d", i); 
    exit(-1);
  }
  return ((double)t.tv_sec + (double) t.tv_nsec/1000000000)*1000;
}

int *start_get_test(void *message) {

  memcached_server_st *servers = NULL;
  memcached_st *memc;
  memcached_return rc;
  char key[20];
  long errors_count=0,succesfull_count=0,count=size;
  char *retrieved_value;
  size_t value_length;
  uint32_t flags;
  double  first_timer,second_timer;
  double avg_timer=0,best_timer=0,worse_timer=0,current_timer=0;
  int first_flag,i;
  int segments[5];
  int real_size=0;
  memc = memcached_create(NULL);
   

  servers = memcached_server_list_append(servers, server, port, &rc);
  rc = memcached_server_push(memc, servers);

  if (rc != MEMCACHED_SUCCESS){
    fprintf(stderr, "Couldn't add server: %s\n", memcached_strerror(memc, rc));
    exit(1);
  }

  if(memcached_behavior_set(memc,MEMCACHED_BEHAVIOR_USE_UDP,1) != MEMCACHED_SUCCESS)
    perror("Memcached setting UDP: "); 
  
  printf("UDP FLAG SET AS: %d\n",(int)memcached_behavior_get(memc,MEMCACHED_BEHAVIOR_USE_UDP));
   
  flags=MEMCACHED_BEHAVIOR_TCP_NODELAY;

  while(1){
    count=size;
    first_flag=1;
    succesfull_count=0;
    avg_timer=0;
    best_timer=1000;
    worse_timer=0; 
    current_timer=0;
    errors_count=0;
    
    for(i=0;i<5;i++){
      segments[i]=0;
    }
    srand((unsigned int)get_wall_time());
    while(count){
      sprintf(key,"test%d", rand() % spread_range);
      first_timer=get_wall_time();
      retrieved_value = memcached_get(memc, key, strlen(key), &value_length, &flags, &rc);
      second_timer=get_wall_time();
      current_timer=second_timer - first_timer;
     // if (current_timer < 0)
     //    printf("%ld %ld %ld\n", current_timer,second_timer.tv_nsec,first_timer.tv_nsec);
      if((current_timer > worse_timer) || first_flag==1 ) 
         worse_timer=current_timer;
      if((current_timer < best_timer)|| first_flag==1){
         best_timer=current_timer;
         first_flag=0;
      }
      avg_timer+=current_timer;

      if (rc != MEMCACHED_SUCCESS) {
        //fprintf(stderr, "Couldn't retrieve key: %s\n", memcached_strerror(memc, rc));
        errors_count+=1;
      }
      else{
        succesfull_count+=1;
        //printf("The key '%s' returned value '%s'.\n", key, retrieved_value);
        if(real_size <= 0)
          real_size = strlen(retrieved_value);
        free(retrieved_value);
      }


      if(current_timer < 1.0){
         segments[0]++;
      }
      else if(current_timer >= 1.0 && current_timer < 50.0){
         segments[1]++;
      }
      else if (current_timer >= 50.0 && current_timer <100.0){
         segments[2]++;
      }
      else if (current_timer >= 100.0 && current_timer <200.0){
         segments[3]++;
      }
      else{
         segments[4]++;
      }
      count--;
    }

    pthread_mutex_lock( &mutex_stdio );
      printf("Threads: %ld Value size: %d Bytes Error: %ld Success: %ld Avg: %.3f Best:%.3f Worse: %.3f total time: %.3f      seg0: %d seg1: %d seg2: %d seg3: %d seg4: %d \n",
              thread_counter,real_size,errors_count,succesfull_count,(float)(avg_timer/size) , best_timer , worse_timer, avg_timer, segments[0],segments[1],segments[2],segments[3],segments[4]);

      fflush(stdout);

    pthread_mutex_unlock( &mutex_stdio );

  }
  return 0;

}

int main(int argc, char **argv) {
  strncpy(server,argv[1],strlen(argv[1])+1);
  port=atoi(argv[2]);
  package_size=atoi(argv[3]);
  pthread_t threads[MAX_THREADS];
  int message=0,i;
  printf("Server: %s \nPort: %d\n size in Bytes:%d\n ",server,port,package_size);

  fill_value(SPREAD_WRITE_RANGE);  
   
  while(thread_counter < MAX_THREADS){
    thread_counter++;
    pthread_create( &threads[thread_counter-1], NULL, (void*)start_get_test, (void*) message);
    sleep(60*TIMER_SPAWN_THREAD);
  }
 
  for(i=0 ; i<MAX_THREADS;i++){
    pthread_join( threads[i], NULL);
  }
   
  return 0;
}
