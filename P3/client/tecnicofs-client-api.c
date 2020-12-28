#include "tecnicofs-client-api.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>

int sockfd;
char clientName[MAX_INPUT_SIZE];
socklen_t servlen, clilen;
struct sockaddr_un serv_addr, client_addr;

int setSockAddrUn(char *path, struct sockaddr_un *addr) {

  if (addr == NULL)
    return 0;

  bzero((char *)addr, sizeof(struct sockaddr_un));
  addr->sun_family = AF_UNIX;
  strcpy(addr->sun_path, path);

  return SUN_LEN(addr);
}

int tfsCreate(char *filename, char nodeType) {
  int tam;
  char buff[MAX_INPUT_SIZE];
  char out_buffer[MAX_INPUT_SIZE];

  tam = sprintf(buff, "c %s %c", filename, nodeType);

  if (sendto(sockfd, buff, tam+1, 0, (struct sockaddr *) &serv_addr, servlen) < 0) {
    perror("client: sendto error\n");
    exit(EXIT_FAILURE);
  }

  if (recvfrom(sockfd, out_buffer, sizeof(out_buffer), 0, 0, 0) < 0) {
    perror("client: recvfrom error\n");
    exit(EXIT_FAILURE);
  }

  return atoi(out_buffer);
}

int tfsDelete(char *path) {
  int tam;
  char buff[MAX_INPUT_SIZE];
  char out_buffer[MAX_INPUT_SIZE];

  tam = sprintf(buff, "d %s", path);

  if (sendto(sockfd, buff, tam+1, 0, (struct sockaddr *) &serv_addr, servlen) < 0) {
    perror("client: sendto error\n");
    exit(EXIT_FAILURE);
  }

  if (recvfrom(sockfd, out_buffer, sizeof(out_buffer), 0, 0, 0) < 0) {
    perror("client: recvfrom error\n");
    exit(EXIT_FAILURE);
  }

  return atoi(out_buffer);
}

int tfsMove(char *from, char *to) {
  int tam;
  char buff[MAX_INPUT_SIZE];
  char out_buffer[MAX_INPUT_SIZE];

  tam = sprintf(buff, "m %s %s", from, to);

  if (sendto(sockfd, buff, tam+1, 0, (struct sockaddr *) &serv_addr, servlen) < 0) {
    perror("client: sendto error\n");
    exit(EXIT_FAILURE);
  }

  if (recvfrom(sockfd, out_buffer, sizeof(out_buffer), 0, 0, 0) < 0) {
    perror("client: recvfrom error\n");
    exit(EXIT_FAILURE);
  }

  return atoi(out_buffer);
}

int tfsLookup(char *path) {
  int tam;
  char buff[MAX_INPUT_SIZE];
  char out_buffer[MAX_INPUT_SIZE];

  tam = sprintf(buff, "l %s", path);

  if (sendto(sockfd, buff, tam+1, 0, (struct sockaddr *) &serv_addr, servlen) < 0) {
    perror("client: sendto error\n");
    exit(EXIT_FAILURE);
  }

  if (recvfrom(sockfd, out_buffer, sizeof(out_buffer), 0, 0, 0) < 0) {
    perror("client: recvfrom error\n");
    exit(EXIT_FAILURE);
  }

  return atoi(out_buffer);
}

int tfsPrint(char *filename){
  int tam;
  char buff[MAX_INPUT_SIZE];
  char out_buffer[MAX_INPUT_SIZE];

  tam = sprintf(buff, "p %s", filename);

  if (sendto(sockfd, buff, tam+1, 0, (struct sockaddr *) &serv_addr, servlen) < 0) {
    perror("client: sendto error on tfsPrint\n");
    exit(EXIT_FAILURE);
  }

  if (recvfrom(sockfd, out_buffer, sizeof(out_buffer), 0, 0, 0) < 0) {
    perror("client: recvfrom error on tfsPrint\n");
    exit(EXIT_FAILURE);
  }

  return atoi(out_buffer);
}

int tfsMount(char* serverName) {

  if((sockfd=socket(AF_UNIX, SOCK_DGRAM, 0)) < 0){
      perror("client: can't open stream socket");
      return -1;
  }

  strcpy(clientName,"/tmp/client-XXXXXX");
  if (mkstemp(clientName) < 0){
    perror("client: temporary file not generated");
    return -1;
  }

  unlink(clientName);
  clilen = setSockAddrUn (clientName, &client_addr);
  if (bind(sockfd, (struct sockaddr *) &client_addr, clilen) < 0) {
      perror("client: bind error");
      return -1;
  }

  servlen = setSockAddrUn(serverName, &serv_addr);
    
  return 0;
}

int tfsUnmount() {
  close(sockfd);
  unlink(clientName);
  return 0;
}
