#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
// Implicit declaration solved: https://stackoverflow.com/questions/11049687/something-about-inet-ntoa
#include <arpa/inet.h>

#define fflush(stdin) while(getchar() != '\n');

#define SERV_PORT 6990
#define BUFFER_SIZE 1024