#include <libmemcached/memcached.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

long size=10000;


int fill_value(char *value,int size){
  int i=0;
  for (i=0; i < size; i++){
    value[i]='A';
  }
  value[i]='\0';
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


int main(int argc, char **argv) {
  memcached_server_st *servers = NULL;
  memcached_st *memc;
  memcached_return rc;
  char *key = "test";
  char value[1000000];  //The default maximum object size is 1MB in memcached
  char server[50];
  int  port=0;
  long errors_count=0,succesfull_count=0,count=size;
  char *retrieved_value;
  size_t value_length;
  uint32_t flags;
  double  first_timer,second_timer;
  double avg_timer=0,best_timer=0,worse_timer=0,current_timer=0;
  int first_flag,i;
  int segments[5];
  int package_size=0,real_size=0;
  strncpy(server,argv[1],strlen(argv[1])+1);
  port=atoi(argv[2]);
  memc = memcached_create(NULL);
   
  package_size=atoi(argv[3]);
  
  fill_value(value,package_size);  

  printf("Server: %s \nPort: %d\n size in Bytes:%d\n ",server,port,package_size);

  servers = memcached_server_list_append(servers, server, port, &rc);
  rc = memcached_server_push(memc, servers);

  if (rc != MEMCACHED_SUCCESS){
    fprintf(stderr, "Couldn't add server: %s\n", memcached_strerror(memc, rc));
    exit(1);
  }

  rc = memcached_set(memc, key, strlen(key), value, strlen(value), (time_t)0, MEMCACHED_BEHAVIOR_TCP_NODELAY);

  if (rc != MEMCACHED_SUCCESS){
    fprintf(stderr, "Couldn't store key: %s\n", memcached_strerror(memc, rc));
    exit(1);
  }

  flags=MEMCACHED_BEHAVIOR_TCP_NODELAY;
  while(1){
    count=size;
    first_flag=1;
    succesfull_count=0;
    avg_timer=0;
    best_timer=1000;
    worse_timer=0; 
    current_timer=0;
    
    for(i=0;i<5;i++){
      segments[i]=0;
    }

    while(count){
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
      else if (current_timer >= 100.0 && current_timer <150.0){
         segments[3]++;
      }
      else if (current_timer >= 200.0){
         segments[4]++;
      }
      count--;
    }
    printf("Value size: %d Bytes Error: %ld Success: %ld Avg: %.3f Best:%.3f Worse: %.3f total time: %.3f      seg0: %d seg1: %d seg2: %d seg3: %d seg4: %d \n",
            real_size,errors_count,succesfull_count,(float)(avg_timer/size) , best_timer , worse_timer, avg_timer, segments[0],segments[1],segments[2],segments[3],segments[4]);

    fflush(stdout);
  }
  return 0;
}
