#include "synchronization.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

/*Variaveis globais */
pthread_mutex_t mutex;
pthread_rwlock_t rwlock;

/*Variavel externa global*/
extern int strategy;

void initLocks(){
	if (strategy == RWLOCK){
        if (pthread_mutex_init(&mutex,NULL)!=0 || pthread_rwlock_init(&rwlock,NULL)!=0){
			fprintf(stderr, "Error: Mutex and rwlock not created\n");
        	exit(EXIT_FAILURE);
    	}
	}
	else if (strategy == MUTEX){
		if (pthread_mutex_init(&mutex,NULL)!=0){
			fprintf(stderr, "Error: Mutex not created\n");
        	exit(EXIT_FAILURE);
		}
	}
}

void lockCommands(){
	if (strategy != NOSYNC){
		if (pthread_mutex_lock(&mutex)!=0){
			fprintf(stderr, "Error: Mutex not locked\n");
        	exit(EXIT_FAILURE);
		}
    }
}

void unlockCommands(){
	if (strategy != NOSYNC){
		if (pthread_mutex_unlock(&mutex)!=0){
			fprintf(stderr, "Error: Mutex not unlocked\n");
        	exit(EXIT_FAILURE);
		}
    }
}

void lockWrite(){
	if (strategy == RWLOCK){
		if (pthread_rwlock_wrlock(&rwlock)!=0){
            fprintf(stderr, "Error: RW lock to write not locked\n");
        	exit(EXIT_FAILURE);
        }
    }
	else if (strategy == MUTEX){
		if (pthread_mutex_lock(&mutex)!=0){
            fprintf(stderr, "Error: Mutex not unlocked\n");
        	exit(EXIT_FAILURE);
        }
    }
}

void lockRead(){
	if (strategy == RWLOCK){
		if (pthread_rwlock_rdlock(&rwlock)!=0){
            fprintf(stderr, "Error: RW lock to read not locked\n");
        	exit(EXIT_FAILURE);
        }
    }
	else if (strategy == MUTEX){
        if (pthread_mutex_lock(&mutex)!=0){
            fprintf(stderr, "Error: Mutex not unlocked\n");
        	exit(EXIT_FAILURE);
        }
    }	
}

void unlockRW(){
	if (strategy == RWLOCK){
		if (pthread_rwlock_unlock(&rwlock)!=0){
            fprintf(stderr, "Error: RW lock not unlocked\n");
        	exit(EXIT_FAILURE);
        }
    }
	else if (strategy == MUTEX){
        if (pthread_mutex_unlock(&mutex)!=0){
			fprintf(stderr, "Error: Mutex not unlocked\n");
        	exit(EXIT_FAILURE);
		}
    }	
}

void destroyLocks(){
	if (strategy == RWLOCK){
        if (pthread_mutex_destroy(&mutex)!=0 || pthread_rwlock_destroy(&rwlock)!=0){
            fprintf(stderr, "Error: Mutex and rwlock not destroyed\n");
        	exit(EXIT_FAILURE);
        }   
	}
	else if (strategy == MUTEX){
		if (pthread_mutex_destroy(&mutex)!=0){
            fprintf(stderr, "Error: Mutex not destroyed\n");
        	exit(EXIT_FAILURE);
        }
    }
}