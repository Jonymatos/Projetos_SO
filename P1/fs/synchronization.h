#ifndef SYNCHRONIZATION_H
#define SYNCHRONIZATION_H

#define RWLOCK 2
#define MUTEX 1
#define NOSYNC -1

/*Inicializacao */
void initLocks();

/*Funcoes que controlam o acesso ao vetor dos Comandos*/
void lockCommands();
void unlockCommands();

/*Funcao que controla a escrita do FS*/
void lockWrite();

/*Funcao que desbloqueia a leitura e escrita do FS*/
void unlockRW();

/*Funcao que controla a leitura do FS*/
void lockRead();

/*Destruicao*/
void destroyLocks();

#endif