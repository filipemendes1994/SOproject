#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/mman.h>	
#include	<sys/wait.h>
#include	<fcntl.h>
#include	<stdio.h>	
#include	<stdlib.h>	
#include	<string.h>	
#include	<unistd.h>
#include	<limits.h>	
#include 	<pthread.h>
#include 	<assert.h>
#include 	<semaphore.h>

#define PIPE_NAME "client_server"

struct stat statbuf;
char *src;

typedef struct {
	char path[1024];
	pid_t pid;
	int rot;
	size_t size;
}cliente;

int get_stat(int fdin);

void sig1(int signum){
	printf("\nYour rotation was sucesseful\n");
	exit(0);
}
void sig2(int signum){
	printf("\nYour rotation was unsucesseful please try again\n");
	exit(0);
}
void sig3(int signum){
	printf("ERROR\n");
	unlink(PIPE_NAME);
	exit(0);
}
/*void sig4(int signum){
	printf("Pipe is busy waiting on pipe\n");
}*/

int main(int argc, char *argv[]){

	int fdin,fd;
	signal(SIGUSR1,sig1);
	signal(SIGUSR2,sig2);
	signal(SIGINT,sig3);
	//signal(SIGPIPE,sig4);
	
	if (argc != 3)
	{
		fprintf(stderr,"Too many or too few arguments\n");
		exit(0);
	}
	if ( (fdin = open(argv[1], O_RDONLY)) < 0)//----verificaçoa da path
	{
		fprintf(stderr,"can't open path for reading\n");
		exit(0);
	}
	if ( (fd = open(PIPE_NAME, O_WRONLY)) < 0)
	{
		fprintf(stderr,"can't open pipe for writing \n");
		exit(0);
	}



	cliente n;	
	n.size = get_stat(fdin);

	if ( (src = mmap(0, n.size, PROT_READ, MAP_FILE | MAP_SHARED, fdin, 0)) == (caddr_t) -1)
	{
		fprintf(stderr,"ERROR: mmap error for input\n");
		exit(1);
	}
	munmap(src,n.size);

	close(fdin);
	n.pid= getpid();
	n.rot= atoi(argv[2]);
	strcpy(&n.path[0],argv[1]);
	//printf("%s\n", n.path);
	
	write(fd, &n,sizeof(cliente)); 
	close(fd);
	unlink(PIPE_NAME);
 
	while(1){
		printf("Waiting for rotation.......");
		sleep(10);
	}
	exit(0);
}

int get_stat(int fdin)
{
	struct stat pstatbuf;	
	if (fstat(fdin, &pstatbuf) < 0)	/* need size of input file */
	{
		fprintf(stderr,"fstat error\n");
		exit(1);
	}
	return pstatbuf.st_size;
}
