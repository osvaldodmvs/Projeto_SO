#ifndef	_FUNCTIONS_H
#define	_FUNCTIONS_H

#include "apue.h"
#include <ctype.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <time.h>
#include <threads.h>
#include <unistd.h>

#define MAX1024 1024
#define MAX100 100
#define MAX50 50
#define MAX20 20
#define MAX10 10
#define M 8
#define N 20
#define N_SALAS 4

#define BUF_SIZE 4096
#define LISTENQ 10

struct tm formatted_timing;
struct tm timestamp_timing;

typedef struct line{
	long admissao;
	long inicio_triagem;
	long fim_triagem;
	long inicio_medico;
	long fim_medico;
}LINE;

typedef struct product{
	long timestamp;
	long ocupacao;
	char sala[MAX50];
}PRODUCT;

int return_year_formatted_tstamp(char * msg);
int return_year_tstamp(long timestamp);
int number_years(LINE * temp, int N_LINHAS);

#endif	/* _FUNCTIONS_H */
