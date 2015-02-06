#include "ProjectoSO.h"

void sendSignal(validade last){
	if(last.rotacaofeita==0)
 		kill(last.pid, SIGUSR1);
 	else if(last.rotacaofeita==1)
 		kill(last.pid, SIGUSR2);
}

void sigint(int signum){
	int i;
	unlink(PIPE_NAME);
	pthread_mutex_destroy(&mutex);
	
	for(i=0;i<6;i++){
		close(pipesReceive[i]);
		close(pipesSend[i]);
	}

	for(i=0;i<3;i++){
		kill(childs[i], SIGKILL);
	}

	printf("Processes are dead,Server shuting down....bye\n");
	exit(0);
}

void set_handlers() { 
	sigset_t mask, unmask; 
	sigfillset(&mask); 
	sigprocmask(SIG_SETMASK, &mask, NULL); 
	sigemptyset(&unmask); 
	sigaddset(&unmask, SIGINT); 
	sigaddset(&unmask, SIGUSR1); 
	sigaddset(&unmask, SIGUSR2); 
}

void server_descriptors(){

	pipesReceive[0] = receive90[0];
	pipesReceive[1] = receive90[1];
	pipesReceive[2] = receive180[0];
	pipesReceive[3] = receive180[1];
	pipesReceive[4] = receive270[0];
	pipesReceive[5] = receive270[1];

	pipesSend[0] = send90[0];
	pipesSend[1] = send90[1];
	pipesSend[2] = send180[0];
	pipesSend[3] = send180[1];
	pipesSend[4] = send270[0];
	pipesSend[5] = send270[1];

	close(receive90[1]);
	close(send90[0]);
	close(receive180[1]);
	close(send180[0]);
	close(receive270[1]);
	close(send270[0]);
 	
}

void init(){
	if(pipe(receive90)==0 && pipe(send90)==0){
		childs[0]=fork();
		if(childs[0]==0)
			master_thread(90);	
		printf("------- ROTAÇÃO DE 90º PRONTA A FUNCIONAR -------\n");
	}	
	if(pipe(receive180)==0 && pipe(send180)==0){
		childs[1]=fork();
		if(childs[1]==0)
			master_thread(180);	
		printf("------- ROTAÇÃO DE 180º PRONTA A FUNCIONAR -------\n");
	}	
	if(pipe(receive270)==0 && pipe(send270)==0){
		childs[2]=fork();
		if(childs[2]==0)
			master_thread(270);	
		printf("------- ROTAÇÃO DE 270º PRONTA A FUNCIONAR -------\n");
	}	
}

int main(){

	int fd;
	int i;
	validade response;

	signal(SIGINT, sigint);

	init();
	server_descriptors();

	//---------- ALL WORK BEHIND SERVER ---------------
	if ((mkfifo(PIPE_NAME, O_CREAT|O_EXCL|0600)<0) &&  (errno!= EEXIST)){
		perror("Cannot create pipe: "); 	
		exit(0);
	}	

	printf("------- NAMED PIPE CRIADO ---------\n\n");

	if ((fd = open(PIPE_NAME, O_RDWR | O_NONBLOCK)) < 0) { 	
		perror("Cannot open pipe for reading: "); 	
		exit(0); 	
	} 	
  	// -------------- ALL WORK FORWARD SERVER ----------

	
	//printf("PIPE:%d\nPIPE1: %d\nPIPE2: %d\nPIPE3: %d\n", fd,receive90[0],receive180[0], receive270[0]);	  
	while (1) { 
		//printf("entrou no while\n");
		fd_set read_set;	
		FD_ZERO(&read_set);

		FD_SET(fd, &read_set); 
		FD_SET(receive90[0], &read_set); 
		FD_SET(receive180[0], &read_set); 
		FD_SET(receive270[0], &read_set); 

 		if (select(fd+1, &read_set, NULL, NULL, NULL) > 0 ) { 	
 			//printf("entrou no select\n");
 			if (FD_ISSET(fd, &read_set)) {
 				
				cliente tmp;
				//printf("select\n");
				read(fd, &tmp, sizeof(cliente));
				//close(fd);
				/*if ((fd = open(PIPE_NAME, O_RDWR | O_NONBLOCK)) < 0) { 	
					perror("Cannot open pipe for reading: "); 	
					exit(0); 	
				} 	*/
				//printf("leu do named\n");
				printf("----- RECEBEU DO CLIENTE -----\nFICHEIRO: %s\nROTAÇAO: %d\n", tmp.path, tmp.rot);
				//printf("-->select %s, %d\n", tmp.path, tmp.rot);
				//id = tmp.pid;

				if(tmp.rot==90) 
					i=0;
				else if(tmp.rot==180) 
					i=2;
				else if(tmp.rot==270) 
					i=4;

				write(pipesSend[i+1], &tmp, sizeof(cliente));
				printf("----- PEDIDO ENVIADO A PROCESSO FILHO -----\n");
		 	}
 			

 			for(i=0;i<6;i=i+2){
 				if (FD_ISSET(pipesReceive[i], &read_set)){
 					//close(pipesReceive[i+1]);
 					read(pipesReceive[i],&response, sizeof(validade));
 					//printf("resposta %d\n", response.rotacaofeita);
 					sendSignal(response);
 				}
 			}


 			//printf("fd\n");
		}
	}
	return 0;
}

//---------------------------------- THREADS ----------------------------


void close_descriptors(int rotation){
	if(rotation==90){
 		close(receive90[0]);
 		close(send90[1]);
 	}
 	else if(rotation==180){
 		close(receive180[0]);
 		close(send180[1]);
 	}
 	else{
 		close(receive270[0]);
 		close(send270[1]);
 	}
}

void* consumer(void * pid){
	int successfullRotation=0;

	while(1){
	   	sem_wait(&full);
	   	//printf("cheguei1!\n");
	    pthread_mutex_lock(&mutex);
	    //printf("passei o mutex\n");

	    cliente tmp = buf[read_pos];
	    read_pos = (read_pos+1) % N; 

	    printf("IMAGEM %s A RODAR\n\n\n", tmp.path);
	   	successfullRotation=do_rotation(tmp);
	   //	printf("-------> thread %s, %d\n", tmp.path, tmp.rot);
	   	validade rotation;
	   	rotation.pid = tmp.pid;
	   	rotation.rotacaofeita=successfullRotation;

	   	if(tmp.rot==90){
			//close(receive90[0]);
			write(receive90[1], &rotation, sizeof(validade));
			printf("RESPOSTA ENVIADA PARA ROTAÇAO DE 90º\n");
		}

		else if(tmp.rot==180){
			//close(receive180[0]);
			write(receive180[1], &rotation, sizeof(validade));
			printf("RESPOSTA ENVIADA PARA ROTAÇAO DE 180º\n");
		}

		else{
			//close(receive270[0]);
			write(receive270[1], &rotation, sizeof(validade));
			printf("RESPOSTA ENVIADA PARA ROTAÇAO DE 270º\n");
		}

	    pthread_mutex_unlock(&mutex);
	    sem_post(&empty);
	    
	}

	return NULL;
}


void master_thread(int rotation){
	int j;

	sem_init(&empty, 0, MAXFILA);
 	sem_init(&full, 0, 0);
	   
 	write_pos = read_pos = 0;

 	close_descriptors(rotation);

	for(j=0;j<4;j++){
		id[j]=j;
		pthread_create(&tid[j], NULL, consumer, &id[j]);
	}

	while(1){ 
		cliente msg;
	    if(rotation==90){
			//close(send90[1]);
			read(send90[0], &msg, sizeof(cliente));
			printf("PEDIDO RECEBIDO PARA ROTAÇAO DE 90º\n");
		}

		else if(rotation==180){
			//close(send180[1]);
			read(send180[0], &msg, sizeof(cliente));
			printf("PEDIDO RECEBIDO PARA ROTAÇAO DE 180º\n");
		}

		else{
			//close(send270[1]);
			read(send270[0], &msg, sizeof(cliente));
			printf("PEDIDO RECEBIDO PARA ROTAÇAO DE 270º\n");
		}

		//sem_getvalue(&empty, &value); 
      	//printf("empty %d\n", value);
		sem_wait(&empty);
		//printf("chegou antes\n");
	    pthread_mutex_lock(&mutex);
	    //printf("chegou antes\n");
		//printf("%s\n",msg.path);
		//printf("worker %s\n",msg);

		//printf("-->worker %s, %d, %d\n", msg.path, msg.rot, rotation);

		buf[write_pos] = msg;
	    write_pos = (write_pos+1) % N;

		sem_post(&full);
		//printf("qwerty\n");
	    pthread_mutex_unlock(&mutex);
	    //printf("qwerty2\n");
	}

	//for (i=0; i<N; i++)
		//pthread_join (tid[i], NULL);

	//pthread_exit(NULL);
}

//------------------ ROTATION WORK .----------------

struct pixel* get_pixel(char *buf, int *pos)
{
	struct pixel pix;
	struct pixel *ppix = &pix;
	ppix->R = buf[*pos];
	ppix->G = buf[(*pos)+1];
	ppix->B = buf[(*pos)+2];
	(*pos) += 3;
	return ppix;
}

void write_pixel(struct pixel *ppix, char *buf, int *pos)
{
	buf[*pos] = ppix->G;
	buf[*(pos)+1] = ppix->B;
	buf[*(pos)+2] = ppix->R;
	(*pos) += 3;
}

int do_rotation(cliente tmp)
{

	char fich[tmp.size];
	int i=0;
	int counter,index, pos, x, y;
	int xmax = 0;
	int ymax = 0;
	int colormax = 0;
	int n=0;

	int fdin2;
	char *src;
	int fdin ;
	char *dst;

	count=rand()%10000;
	sprintf(fich, "novaimagem%d_%s", count, tmp.path);

	if ( (fdin2 = open(tmp.path, O_RDONLY)) < 0)
	{
		fprintf(stderr,"can't open for reading\n");
		return i=1;
	}
	if ( (src = mmap(0, tmp.size, PROT_READ, MAP_FILE | MAP_SHARED, fdin2, 0)) == (caddr_t) -1)
	{
		fprintf(stderr,"mmap error for input\n");
		return i=1;
	}
	if ( (fdin = open(fich, O_RDWR | O_CREAT | O_TRUNC,FILE_MODE)) < 0)
	{
		fprintf(stderr,"can't creat %s for writing\n", fich);
		return i=1;
	}

	if (lseek(fdin, tmp.size - 1, SEEK_SET) == -1)
	{
		fprintf(stderr,"lseek error\n");
		exit(1);
	}
	if (write(fdin, "", 1) != 1)
	{
		fprintf(stderr,"write error\n");
		exit(1);
	}

	if ( (dst= mmap(0, tmp.size, PROT_READ | PROT_WRITE,MAP_SHARED,fdin, 0)) == (caddr_t) -1)
	{
		fprintf(stderr,"mmap error for output\n");
		return i=1;
	}
	//printf("chega aqui");
	//-----verificar map src ou fazer nas threads ou nesta funçao

	sscanf(src,"P6\n%d %d\n%d\n",&xmax,&ymax,&colormax);
	
	struct pixel imagem [ymax][xmax];

	for (counter=0, index=0; counter<3;index++)
	{
		if (src[index]=='\n')
			++counter;
	} 	
	pos=index-1;
	for (y=0;y<ymax;y++)
		for (x=0;x<xmax;x++)
			imagem[y][x] = *(get_pixel(src,&pos));
	pos=index;
	
	switch (tmp.rot)
	{
		case 90 :
			//Rotação 90º clockwise
			sprintf(dst,"P6\n%d %d\n%d\n",ymax,xmax,colormax);
			for (x=0; x<xmax; x++)
				for (y=ymax-1; y> -1;y--)
					write_pixel(&(imagem[y][x]),dst,&pos);
			write(receive90[1],&n,sizeof(int));
			break;
		case 180 :
			//Rotação 180º
			sprintf(dst,"P6\n%d %d\n%d\n",xmax,ymax,colormax);
			for (y=ymax-1;y>-1;y--)
				for (x = xmax-1; x>-1; x--)
					write_pixel(&(imagem[y][x]),dst,&pos);
			write(receive180[1],&n,sizeof(int));
			break;
		case 270 :
			//Rotação 270º clockwise
			sprintf(dst,"P6\n%d %d\n%d\n",ymax,xmax,colormax);
			for (x=xmax-1; x>-1; x--)
				for (y=0; y<ymax;y++)
					write_pixel(&(imagem[y][x]),dst,&pos);
			write(receive270[1],&n,sizeof(int));
			break;

	}
	munmap(dst,tmp.size);
	munmap(src,tmp.size);
	close(fdin);
	close(fdin2);
	return i=0;
}
