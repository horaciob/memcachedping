all:    
	gcc memcachedping.c -o test  -lmemcached -lrt -Wall
