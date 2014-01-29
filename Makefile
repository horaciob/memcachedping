all:    
	gcc memcachedping.c -o test -g -lmemcached -lrt -Wall
