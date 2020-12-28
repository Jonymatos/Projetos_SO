#include "state.h"
#include "synchronization.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

/*Variaveis globais */
extern pthread_mutex_t mutex;
extern pthread_cond_t podeCons, podeProd;
extern inode_t inode_table[INODE_TABLE_SIZE];

void initLocks(){
    if (pthread_mutex_init(&mutex,NULL)!=0 ){
		fprintf(stderr, "Error: Mutex not created\n");
        exit(EXIT_FAILURE);
    }

    if (pthread_cond_init(&podeCons, NULL) != 0 || pthread_cond_init(&podeProd, NULL) != 0) {
        fprintf(stderr, "Error creating cond thread.\n");
        exit(EXIT_FAILURE);
    }

}

void lockMutex(){
	if (pthread_mutex_lock(&mutex)!=0){
		fprintf(stderr, "Error: Mutex not locked\n");
        exit(EXIT_FAILURE);
	}
}

void unlockMutex(){
	if (pthread_mutex_unlock(&mutex)!=0){
		fprintf(stderr, "Error: Mutex not unlocked\n");
        exit(EXIT_FAILURE);		
    }
}

int lockWrite(int *sync, int inumber){
    if (checkInumber(sync, inumber)){
        return 0;
    }
	if (pthread_rwlock_wrlock(&inode_table[inumber].rwlock)!=0){
        fprintf(stderr, "Error: RW lock to write not locked\n");
        exit(EXIT_FAILURE);
    }
    return 1;   
}

int lockRead(int *sync, int inumber){
    if (checkInumber(sync, inumber)){
        return 0;
    }
	if (pthread_rwlock_rdlock(&inode_table[inumber].rwlock)!=0){
        fprintf(stderr, "Error: RW lock to read not locked\n");
        exit(EXIT_FAILURE);
    }
    return 1;
}

void unlockRW(int inumber){
	if (pthread_rwlock_unlock(&inode_table[inumber].rwlock)!=0){
        fprintf(stderr, "Error: RW lock not unlocked\n");
        exit(EXIT_FAILURE);
    }
}

void unlockAndResetSync(int *sync, int* key){
	for(int i=*key;i>=0;i--){
        if (sync[i]==-1)
            continue;
		unlockRW(sync[i]);
		sync[i]=-1;
	}
    *key = 0;
}

int checkInumber(int *sync, int inumber){
    for (int i=0;i<INODE_TABLE_SIZE && sync[i]!=-1;i++){
        if (sync[i]==inumber){
            return 1;
        }
    }
    return 0;
}

void destroyLocks(){
    if (pthread_mutex_destroy(&mutex)!=0){
        fprintf(stderr, "Error: Mutex not destroyed\n");
    	exit(EXIT_FAILURE);  
	}

    if (pthread_cond_destroy(&podeCons) != 0 || pthread_cond_destroy(&podeProd) != 0) {
        fprintf(stderr, "Error destroying cond thread.\n");
        exit(EXIT_FAILURE);
    }
	
}