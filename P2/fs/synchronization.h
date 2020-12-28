#ifndef SYNCHRONIZATION_H
#define SYNCHRONIZATION_H

/*
#define RWLOCK 2
#define MUTEX 1
#define NOSYNC -1
*/

/*Inicializacao */
void initLocks();

/*Funcoes que controlam o acesso ao vetor dos Comandos*/
void lockMutex();
void unlockMutex();

/*Funcao que controla a escrita do FS*/
int lockWrite(int *sync, int inumber);

/*Funcao que desbloqueia a leitura e escrita do FS*/
void unlockRW(int inumber);

/*Funcao que controla a leitura do FS*/
int lockRead(int *sync, int inumber);

/*Funcao que da unlock aos respetivos inumber no vetor*/
void unlockAndResetSync(int *sync, int* key);

/*Funcao que verifica se um inumber ja estava bloqueado*/ 
int checkInumber(int *sync, int inumber);

/*Destruicao*/
void destroyLocks();

#endif