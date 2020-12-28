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

#define MAX_COMMANDS 10
#define MAX_INPUT_SIZE 100
#define MAX_PTHREADS 100

int numberThreads = 0;

char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];
int count = 0, prodPtr = 0, consPtr = 0;


struct timeval startTime;
struct timeval stopTime;

pthread_mutex_t mutex;
pthread_cond_t podeCons, podeProd;
pthread_t prod;
pthread_t tid[MAX_PTHREADS];

int checkArgs(int argc, char* buffer){
    int threads = atoi(buffer);
    if (argc==4 && threads>=1 && threads<=MAX_PTHREADS)
        return threads;
    else if (threads<=0 || threads>MAX_PTHREADS){
        fprintf(stderr, "Error: Invalid number of pthreads\n");
        exit(EXIT_FAILURE);
    }
    else{
        fprintf(stderr, "Error: Invalid format\n");
        exit(EXIT_FAILURE);
    }
}

int **createArraySync(){
    int i,j, **TableThreads = (int**)malloc(numberThreads*sizeof(int*));
    for(i=0;i<numberThreads;i++){
        TableThreads[i]=(int*)malloc(INODE_TABLE_SIZE*sizeof(int));
        for (j=0;j<INODE_TABLE_SIZE;j++)
            TableThreads[i][j]=-1;
    }
    return TableThreads;
}

void deleteArray(int **TableThreads){
    int i;
    for(i=0;i<numberThreads;i++)
        free(TableThreads[i]);
    free(TableThreads);
}

int insertCommand(char* data) {
    strcpy(inputCommands[prodPtr], data);
    prodPtr = (prodPtr + 1) % MAX_COMMANDS;
    count++;
    return 1;
}

/*char* removeCommand() {
    char command[MAX_INPUT_SIZE];
    strcpy(command, inputCommands[consPtr]);
    consPtr = (consPtr + 1) % MAX_COMMANDS;
    count--;
    return command;
}*/

void errorParse(){
    fprintf(stderr, "Error: command invalid\n");
    exit(EXIT_FAILURE);
}

void *processInput(void* ptr){
    char line[MAX_INPUT_SIZE];
    char* buffer = *((char**)ptr);

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

        lockMutex();
        while (count == MAX_COMMANDS){
            if (pthread_cond_wait(&podeProd, &mutex) != 0){
                fprintf(stderr, "Error on waiting\n");
                exit(EXIT_FAILURE);
            }
        }

        /* perform minimal validation */
        if (numTokens < 1) {
            unlockMutex();
            continue;
        }

        switch (token) {
            case 'c':
                if(numTokens != 3)
                    errorParse();
                if(insertCommand(line)){
                    if (pthread_cond_signal(&podeCons)!=0){
                        fprintf(stderr, "Error on waiting\n");
                        exit(EXIT_FAILURE);
                    }
                    unlockMutex();
                    break;
                }
                return NULL;
            
            case 'l':
                if(numTokens != 2)
                    errorParse();
                if(insertCommand(line)){
                    if (pthread_cond_signal(&podeCons)!=0){
                        fprintf(stderr, "Error on waiting\n");
                        exit(EXIT_FAILURE);
                    }
                    unlockMutex();
                    break;
                }
                return NULL;
            
            case 'd':
                if(numTokens != 2)
                    errorParse();
                if(insertCommand(line)){
                    if (pthread_cond_signal(&podeCons)!=0){
                        fprintf(stderr, "Error on waiting\n");
                        exit(EXIT_FAILURE);
                    }
                    unlockMutex();
                    break;
                }
                return NULL;
            
            case 'm':
                if(numTokens != 3)
                    errorParse();
                if(insertCommand(line)){
                    if (pthread_cond_signal(&podeCons)!=0){
                        fprintf(stderr, "Error on waiting\n");
                        exit(EXIT_FAILURE);
                    }
                    unlockMutex();
                    break;
                }
                return NULL;
            
            case '#':
                unlockMutex();
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

    /*Por cada tarefa consumidora adiciona EOF no comando */
    for(int i=0;i<numberThreads;i++){
        lockMutex();

        while (count == MAX_COMMANDS){
            if (pthread_cond_wait(&podeProd, &mutex) != 0){
                fprintf(stderr, "Error on waiting\n");
                exit(EXIT_FAILURE);
            }
        }

        inputCommands[prodPtr][0]=EOF;
        prodPtr = (prodPtr + 1) % MAX_COMMANDS;
        count++;
        if (pthread_cond_signal(&podeCons) != 0){
            fprintf(stderr, "Error on signaling\n");
            exit(EXIT_FAILURE);
        }
        unlockMutex();
    }
    
    return NULL;
}

void *applyCommands(void *array){
    int key = 0;
    int* sync = *((int**)array);
    char command[MAX_INPUT_SIZE];
    while (1){

        /*Vetor protegido pelo mutex*/
        lockMutex();

        while (count == 0){
            if (pthread_cond_wait(&podeCons, &mutex) != 0){
                fprintf(stderr, "Error on waiting\n");
                exit(EXIT_FAILURE);
            }
        }

        strcpy(command, inputCommands[consPtr]);
        consPtr = (consPtr + 1) % MAX_COMMANDS;
        count--;
        if (pthread_cond_signal(&podeProd) != 0){
            fprintf(stderr, "Error on waiting\n");
            exit(EXIT_FAILURE);
        }

        if (command == NULL){
            unlockMutex();
            return NULL;
        }
        
        unlockMutex();

        char token, type;
        char name[MAX_INPUT_SIZE], destiny[MAX_INPUT_SIZE];
        int numTokens = sscanf(command, "%c %s %c", &token, name, &type);

        /*Le o destino do pathname*/
        if (token=='m')
            numTokens = sscanf(command, "%c %s %s", &token, name, destiny);

        /* Termina a thread*/
        if (token==EOF)
            return NULL;

        if (numTokens < 2 ) {
            fprintf(stderr, "Error: invalid command in Queue\n");
            exit(EXIT_FAILURE);
        }

        int searchResult;
        switch (token) {
            case 'c':
                switch (type) {
                    case 'f':
                        printf("Create file: %s\n", name);
                        create(name, T_FILE, sync, CREATE, &key);
                        break;
                    case 'd':
                        printf("Create directory: %s\n", name);
                        create(name, T_DIRECTORY, sync, CREATE, &key);
                        break;
                    default:
                        fprintf(stderr, "Error: invalid node type\n");
                        exit(EXIT_FAILURE);
                }
                break;
            case 'l': 
                searchResult = lookup(name, sync, LOOKUP, &key);
                if (searchResult >= 0)
                    printf("Search: %s found\n", name);
                else
                    printf("Search: %s not found\n", name);
                break;
            case 'd':
                printf("Delete: %s\n", name);
                delete(name, sync, DELETE, &key);
                break;
            case 'm':
                printf("Move %s to %s\n", name, destiny);
                move(name, destiny, sync, &key);
                break;
            default: { /* error */
                fprintf(stderr, "Error: command to apply\n");
                exit(EXIT_FAILURE);
            }
        }
    }
    /*Erro*/
    fprintf(stderr, "Error: thread wasn't suppose to left the loop \n");
    exit(EXIT_FAILURE);
}

int main(int argc, char* argv[]) {

    /*Verifica se a formatacao e valida implementando o numero de tarefas */
    numberThreads = checkArgs(argc, argv[3]);

    /* init filesystem */
    init_fs();

    /*Inicializacao do mutex e/ou do trico leitura escrita*/
    initLocks();
    int **TableThreads = createArraySync();


    /* inicio da contagem do tempo */
    if(gettimeofday(&startTime, NULL) == -1){
    	fprintf(stderr,"Error on getting startTime!");
    	exit(EXIT_FAILURE);
    }

    /* Criacao da tarefa produtore*/
    if (pthread_create(&prod, 0, processInput, &argv[1]) !=0){
        fprintf(stderr,"Error on creating producing pthread!\n");
        exit(EXIT_FAILURE);
    }

    /* Criacao da(s) tarefa(s) consumidora(s) */
    for(int i=0; i<numberThreads; i++){
        if(pthread_create(&tid[i], 0, applyCommands, &TableThreads[i]) != 0){
            fprintf(stderr,"Error on creating consumer pthread!\n");
            exit(EXIT_FAILURE); 
        }
    }

    /*Espera da tarefa produtora*/
    if (pthread_join(prod, NULL)!=0){
        fprintf(stderr,"Error on joining producing pthread!\n");
        exit(EXIT_FAILURE);
    }

    /*Ciclo de espera pelas tarefas consumidoras*/
    for(int i=0; i<numberThreads; i++){
        if(pthread_join(tid[i], NULL) !=0){
            fprintf(stderr,"Error on joining consumer pthread!!\n");
            exit(EXIT_FAILURE);
        }
    }


    /* fim da contagem do tempo */
    if(gettimeofday(&stopTime, NULL)){
    	fprintf(stderr,"Error on getting stopTime!\n");
    	exit(EXIT_FAILURE);
    }

    /* tempo de execucao */
    double time = (double) (stopTime.tv_sec - startTime.tv_sec) + (double) (stopTime.tv_usec - startTime.tv_usec)/1000000;

    /*Imprime o conteudo do FS mas so no ficheiro output*/
    print_tecnicofs_tree(argv[2]);
    printf("TecnicoFS completed in %0.4f seconds.\n", time);

    /*Destruicao do mutex e das locksWrite */
    destroyLocks();

    /* release allocated memory */
    destroy_fs();
    deleteArray(TableThreads);

    /*Sai com sucesso sem erros*/
    exit(EXIT_SUCCESS);
}
