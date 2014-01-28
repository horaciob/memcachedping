#include <libmemcached/memcached.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

long size=100000;
int main(int argc, char **argv) {
  memcached_server_st *servers = NULL;
  memcached_st *memc;
  memcached_return rc;
  char *key = "test";
  char *value = "horacio";
  char server[50];
  int  port=0;
  long errors_count=0,succesfull_count=0,count=size;
  char *retrieved_value;
  size_t value_length;
  uint32_t flags;
  struct timespec first_timer,second_timer;
  long avg_timer=0,best_timer=0,worse_timer=0,current_timer=0;
  int first_flag;

  strncpy(server,argv[1],strlen(argv[1])+1);
  port=atoi(argv[2]);
  memc = memcached_create(NULL);
 

  printf("Server %s port %d\n",server,port);

  servers = memcached_server_list_append(servers, server, port, &rc);
  rc = memcached_server_push(memc, servers);

  if (rc != MEMCACHED_SUCCESS){
    fprintf(stderr, "Couldn't add server: %s\n", memcached_strerror(memc, rc));
    exit(1);
  }

  rc = memcached_set(memc, key, strlen(key), value, strlen(value), (time_t)0, (uint32_t)0);

  if (rc != MEMCACHED_SUCCESS){
    fprintf(stderr, "Couldn't store key: %s\n", memcached_strerror(memc, rc));
    exit(1);
  }
  while(1){
    count=size;
    first_flag=1;
    succesfull_count=0;
    avg_timer=0;
    best_timer=1000;
    worse_timer=0; 
    current_timer=0;
    while(count){
      clock_gettime(CLOCK_MONOTONIC_RAW, &first_timer);
      retrieved_value = memcached_get(memc, key, strlen(key), &value_length, &flags, &rc);
      clock_gettime(CLOCK_MONOTONIC_RAW, &second_timer);
      current_timer=second_timer.tv_nsec - first_timer.tv_nsec;
      if((current_timer > worse_timer) || first_flag==1 ) 
         worse_timer=current_timer;
      if((current_timer < best_timer)|| first_flag==1){
         best_timer=current_timer;
         first_flag==0;
      }
      avg_timer+=current_timer;

      if (rc != MEMCACHED_SUCCESS) {
        //fprintf(stderr, "Couldn't retrieve key: %s\n", memcached_strerror(memc, rc));
        errors_count+=1;
      }
      else{
        succesfull_count+=1;
        //printf("The key '%s' returned value '%s'.\n", key, retrieved_value);
        free(retrieved_value);
      }
      count--;
    }
    printf("Total: %ld errores: %ld  success: %ld avg: %ld best:%ld worse: %ld\n",
           count, errors_count,succesfull_count, avg_timer/size , best_timer , worse_timer);
    fflush(stdout);
  }
  return 0;
}
