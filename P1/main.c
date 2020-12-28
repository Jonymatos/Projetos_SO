/*
Grupo 006
95610 Joao Rui Vargas de Matos
58387 Nelson Jose Da Silva Palmeira de Paiva
*/

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <sys/time.h>
#include "fs/synchronization.h"
#include "fs/operations.h"

#define MAX_COMMANDS 150000
#define MAX_INPUT_SIZE 100
#define MAX_PTHREADS 100

int numberThreads = 0;
int strategy;

char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];
int numberCommands = 0;
int headQueue = 0;

struct timeval startTime;
struct timeval stopTime;

pthread_t tid[MAX_PTHREADS];

int checkArgs(int argc, char* buffer){
    int Threads = atoi(buffer);
    if (argc==5 && Threads>=1 && Threads<=MAX_PTHREADS)
        return Threads;
    else if (Threads<=0 || Threads>MAX_PTHREADS){
        fprintf(stderr, "Error: Invalid number of pthreads\n");
        exit(EXIT_FAILURE);
    }
    else{
        fprintf(stderr, "Error: Invalid format\n");
        exit(EXIT_FAILURE);
    }
}

int chooseStrategy(char *buffer){
    if (!strcmp(buffer,"mutex"))
        return MUTEX;

    else if (!strcmp(buffer,"rwlock"))
        return RWLOCK;

    else if (!strcmp(buffer,"nosync") && numberThreads==1)
        return NOSYNC;

    else if (!strcmp(buffer,"nosync") && numberThreads!=1){
        fprintf(stderr, "Error: Invalid number of threads\n");
        exit(EXIT_FAILURE);
    }
        
    else{
        fprintf(stderr, "Error: Strategy not valid\n");
        exit(EXIT_FAILURE);
    } 
}

int insertCommand(char* data) {
    if(numberCommands != MAX_COMMANDS) {
        strcpy(inputCommands[numberCommands++], data);
        return 1;
    }
    return 0;
}

char* removeCommand() {
    if(numberCommands > 0){
        numberCommands--;
        return inputCommands[headQueue++];  
    }
    return NULL;
}

void errorParse(){
    fprintf(stderr, "Error: command invalid\n");
    exit(EXIT_FAILURE);
}

void processInput(char* buffer){
    char line[MAX_INPUT_SIZE];

    /*Le o ficheiro*/
    FILE *input = fopen(buffer,"r");
    
    /*Se nao encontrar o ficheiro*/
    if (!input){
        fprintf(stderr,"Error: file not found\n");
        exit(EXIT_FAILURE);
    }

    while (fgets(line, sizeof(line)/sizeof(char), input)) {
        char token, type;
        char name[MAX_INPUT_SIZE];

        int numTokens = sscanf(line, "%c %s %c", &token, name, &type);

        /* perform minimal validation */
        if (numTokens < 1) {
            continue;
        }
        switch (token) {
            case 'c':
                if(numTokens != 3)
                    errorParse();
                if(insertCommand(line))
                    break;
                return;
            
            case 'l':
                if(numTokens != 2)
                    errorParse();
                if(insertCommand(line))
                    break;
                return;
            
            case 'd':
                if(numTokens != 2)
                    errorParse();
                if(insertCommand(line))
                    break;
                return;
            
            case '#':
                break;
            
            default: { /* error */
                errorParse();
            }
        }
    }
    if (fclose(input)!=0){
        fprintf(stderr,"Error: Impossible to close file\n");
        exit(EXIT_FAILURE);
    }
}

void *applyCommands(){
    while (numberCommands > 0){

        /*Vetor protegido pelo mutex*/
        lockCommands();
        const char* command = removeCommand();
        if (command == NULL){
            unlockCommands();
            continue;
        }

        char token, type;
        char name[MAX_INPUT_SIZE];
        int numTokens = sscanf(command, "%c %s %c", &token, name, &type);
        unlockCommands();

        if (numTokens < 2) {
            fprintf(stderr, "Error: invalid command in Queue\n");
            exit(EXIT_FAILURE);
        }

        int searchResult;
        switch (token) {
            case 'c':
                switch (type) {
                    case 'f':
                        lockWrite();
                        printf("Create file: %s\n", name);
                        create(name, T_FILE);
                        unlockRW();
                        break;
                    case 'd':
                        lockWrite();
                        printf("Create directory: %s\n", name);
                        create(name, T_DIRECTORY);
                        unlockRW();
                        break;
                    default:
                        fprintf(stderr, "Error: invalid node type\n");
                        exit(EXIT_FAILURE);
                }
                break;
            case 'l': 
                lockRead();
                searchResult = lookup(name);
                if (searchResult >= 0)
                    printf("Search: %s found\n", name);
                else
                    printf("Search: %s not found\n", name);
                unlockRW();
                break;
            case 'd':
                printf("Delete: %s\n", name);
                lockWrite();
                delete(name);
                unlockRW();
                break;
            default: { /* error */
                fprintf(stderr, "Error: command to apply\n");
                exit(EXIT_FAILURE);
            }
        }
    }
    return NULL;
}

int main(int argc, char* argv[]) {

    /*Verifica se a formatacao e valida implementando o numero de tarefas */
    numberThreads = checkArgs(argc, argv[3]);

    /*Verifica e implementa a estrategia a ser usada*/
    strategy = chooseStrategy(argv[4]);

    /* init filesystem */
    init_fs();

    /*Inicializacao do mutex e/ou do trico leitura escrita*/
    initLocks();

    /*Le o ficheiro input guardando os comanoos no vetor*/
    processInput(argv[1]);

    /* inicio da contagem do tempo */
    if(gettimeofday(&startTime, NULL) == -1){
    	perror("Error getting startTime!");
    	exit(EXIT_FAILURE);
    }

    /* for com pthread_create até o numero de tarefas */
    for(int i=0; i<numberThreads; i++){
        if(pthread_create(&tid[i], 0, applyCommands, NULL) != 0){
            printf("Erro na criacao da tarefa!\n");
            exit(EXIT_FAILURE); 
        }
    }

    /*Ciclo de espera pelas tarefas*/
    for(int i=0; i<numberThreads; i++){
        if(pthread_join(tid[i], NULL) !=0){
            printf("Erro ao esperar por tarefa!\n");
            exit(EXIT_FAILURE);
        }
    }


    /* fim da contagem do tempo */
    if(gettimeofday(&stopTime, NULL)){
    	perror("Error getting stopTime!");
    	exit(EXIT_FAILURE);
    }

    /* tempo de execucao */
    double time = (double) (stopTime.tv_sec - startTime.tv_sec) + (double) (stopTime.tv_usec - startTime.tv_usec)/1000000;

    /*Imprime o conteudo do FS mas so no ficheiro output*/
    print_tecnicofs_tree(argv[2]);
    printf("TecnicoFS completed in %0.4f seconds.\n", time);

    /*Destruicao do mutex e/ou do trinco leitura-escrita*/
    destroyLocks();

    /* release allocated memory */
    destroy_fs();
    exit(EXIT_SUCCESS);
}
