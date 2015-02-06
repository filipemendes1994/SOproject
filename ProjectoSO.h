#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <pthread.h>
#include <semaphore.h>

#define	FILE_MODE	(S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

#ifndef	MAP_FILE	/* 44BSD defines this & requires it to mmap files */
#define	MAP_FILE	0	/* to compile under systems other than 44BSD */
#endif

#define SIZE 20
#define MAXPATH 1024

#define MAXFILA 30
#define N 4
#define PIPE_NAME "client_server"

//--------------------- STRUCT NAMED PIPE ---------------
typedef struct {
	char path[MAXPATH];
	pid_t pid;
	int rot;
	size_t size;
}cliente;

typedef struct {
	pid_t pid;
	int rotacaofeita;
}validade;

//------------PIPES SERVER/MASTER THREAD-------------
int receive90[2], receive180[2], receive270[2];
int send90[2], send180[2], send270[2];
int pipesReceive[6], pipesSend[6];

//--------------- CHILDS -------------
pid_t childs[3];

//--------------- THREADS -----------------

sem_t empty, full;
int write_pos, read_pos;
pthread_t tid[N];
int id[N];
cliente buf[MAXFILA];
pthread_mutex_t mutex= PTHREAD_MUTEX_INITIALIZER;



//------------ROTATION-----------------

struct stat statbuf;
struct pixel
{
	char R;
	char G;
	char B;
};
int count;


void init();
void server_descriptors();
void set_handlers();
void sigint(int signum);
void sendSignal(validade last);


void master_thread(int rotation);
void* consumer(void * pid);
void close_descriptors(int rotation);

struct pixel* get_pixel(char *buf, int *pos);
void write_pixel(struct pixel *ppix, char *buf, int *pos);
int do_rotation(cliente tmp);
