
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
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <unistd.h>
#include "fs/synchronization.h"
#include "fs/operations.h"

#define MAX_INPUT_SIZE 100
#define MAX_PTHREADS 100

int numberThreads = 0;

int sockfd;
struct sockaddr_un server_addr;
socklen_t addrlen;
char *path;

struct timeval startTime;
struct timeval stopTime;

pthread_mutex_t mutex;
pthread_cond_t podeCons, podeProd;
pthread_t prod;
pthread_t tid[MAX_PTHREADS];

int setSockAddrUn(char *path, struct sockaddr_un *addr) {

  if (addr == NULL)
    return 0;

  bzero((char *)addr, sizeof(struct sockaddr_un));
  addr->sun_family = AF_UNIX;
  strcpy(addr->sun_path, path);

  return SUN_LEN(addr);
}

int checkArgs(int argc, char** argv){
    int threads = atoi(argv[1]);
    if (argc==3 && threads>=1 && threads<=MAX_PTHREADS)
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

void errorParse(){
    fprintf(stderr, "Error: command invalid\n");
    exit(EXIT_FAILURE);
}

void *applyCommands(void *array){
    int c, key = 0;
    int* sync = *((int**)array);
    //Cada tarefa escava tem um andresso do cliente
    struct sockaddr_un client_addr;
    char command[MAX_INPUT_SIZE], toSend[MAX_INPUT_SIZE];
    while (1){
        
        addrlen=sizeof(struct sockaddr_un);
        c = recvfrom(sockfd, command, sizeof(command)-1, 0, (struct sockaddr *)&client_addr, &addrlen);

        char token, type;
        char name[MAX_INPUT_SIZE], destiny[MAX_INPUT_SIZE];
        int numTokens = sscanf(command, "%c %s %c", &token, name, &type);

        /*Le o destino do pathname*/
        if (token=='m')
            numTokens = sscanf(command, "%c %s %s", &token, name, destiny);

        if (numTokens < 2 ) {
            fprintf(stderr, "Error: invalid command in Queue\n");
            exit(EXIT_FAILURE);
        }

        int searchResult;
        switch (token) {
            case 'c':
                switch (type) {
                    case 'f':
                        printf("server: Create file: %s\n", name);
                        searchResult = create(name, T_FILE, sync, CREATE, &key);
                        break;
                    case 'd':
                        printf("server: Create directory: %s\n", name);
                        searchResult = create(name, T_DIRECTORY, sync, CREATE, &key);
                        break;
                    default:
                        fprintf(stderr, "server: Error: invalid node type\n");
                        exit(EXIT_FAILURE);
                }
                break;
            case 'l': 
                searchResult = lookup(name, sync, LOOKUP, &key);
                if (searchResult >= 0)
                    printf("server: Search %s found\n", name);
                else
                    printf("server: Search %s not found\n", name);
                break;
            case 'd':
                searchResult = delete(name, sync, DELETE, &key);
                if (searchResult == SUCCESS)
                    printf("server: Delete %s sucefully done\n", name);
                else
                    printf("server: Delete %s unsucefully not done\n", name);
                break;
            case 'm':
                printf("server: Move %s to %s\n", name, destiny);
                searchResult = move(name, destiny, sync, &key);
                break;
            case 'p':
                searchResult = print_tecnicofs_tree(name, sync, &key);
                if (searchResult != SUCCESS)
                    printf("Server: Error printing to %s.\n", name);
                break;
            default: { /* error */
                fprintf(stderr, "Error: command to apply\n");
                exit(EXIT_FAILURE);
            }
        }
        c = sprintf(toSend,"%d", searchResult);
        sendto(sockfd, toSend, c+1, 0, (struct sockaddr *)&client_addr, addrlen);
    }
    /*Erro*/
    fprintf(stderr, "Error: thread wasn't suppose to left the loop \n");
    exit(EXIT_FAILURE);
}

int main(int argc, char* argv[]) {

    /*Verifica se a formatacao e valida implementando o numero de tarefas */
    numberThreads = checkArgs(argc, argv);

    /* init filesystem */
    init_fs();

    /*Inicializacao do mutex e/ou do trico leitura escrita*/
    initLocks();
    int **TableThreads = createArraySync();


    if ((sockfd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) {
        perror("server: can't open socket");
        exit(EXIT_FAILURE);
    }

    path = argv[2];

    unlink(path);

    addrlen = setSockAddrUn (argv[2], &server_addr);
    if (bind(sockfd, (struct sockaddr *) &server_addr, addrlen) < 0) {
        perror("server: bind error");
        exit(EXIT_FAILURE);
    }

    /* Criacao da(s) tarefa(s) consumidora(s) */
    for(int i=0; i<numberThreads; i++){
        if(pthread_create(&tid[i], 0, applyCommands, &TableThreads[i]) != 0){
            fprintf(stderr,"Error on creating consumer pthread!\n");
            exit(EXIT_FAILURE); 
        }
    }

    while(1){}

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

    /*Destruicao do mutex e das locksWrite */
    destroyLocks();

    /* release allocated memory */
    destroy_fs();
    deleteArray(TableThreads);

    /*Sai com sucesso sem erros*/
    exit(EXIT_SUCCESS);
}
