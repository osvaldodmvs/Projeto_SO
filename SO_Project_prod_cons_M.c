#include "functions.h"

PRODUCT buffer[N];
int prodptr=0, consptr=0;
pthread_mutex_t mutex_prod = PTHREAD_MUTEX_INITIALIZER , mutex_cons = PTHREAD_MUTEX_INITIALIZER;
sem_t can_prod,can_cons;
int N_LINHAS;
int destination;
int PROD=0, CONS=0,N_ANOS=0;
LINE * tmpLines;
LINE * tmpReset;
int condicao_paragem;
int flag_cons;
int flag_prod;

PRODUCT calcular(LINE * i, int sala){
	long tstamp=i->admissao;
	PRODUCT newprod;
	long s_admissao=0,s_triagem=0,s_espera=0,s_consulta=0;
	LINE * mytmpLines=tmpReset;
	switch(sala){
		case 0: for(int k=0;k<N_LINHAS;k++){
			if(mytmpLines->admissao < tstamp && tstamp <= mytmpLines->inicio_triagem && mytmpLines->admissao != 9999 && mytmpLines->inicio_triagem != 9999)s_admissao++;
			mytmpLines++;
		}
		newprod.timestamp=tstamp;
		newprod.ocupacao=s_admissao;
		strcpy(newprod.sala,"espera_triagem");
		break;
		case 1: for(int k=0;k<N_LINHAS;k++){
			if(mytmpLines->inicio_triagem < tstamp && tstamp <= mytmpLines->fim_triagem && mytmpLines->inicio_triagem != 9999 && mytmpLines->fim_triagem != 9999)s_triagem++;
			mytmpLines++;
		}
		newprod.timestamp=tstamp;
		newprod.ocupacao=s_triagem;
		strcpy(newprod.sala,"sala_triagem");
		break;
		case 2: for(int k=0;k<N_LINHAS;k++){
			if(mytmpLines->fim_triagem < tstamp && tstamp <= mytmpLines->inicio_medico && mytmpLines->fim_triagem != 9999 && mytmpLines->inicio_medico != 9999)s_espera++;
			mytmpLines++;
		}
		newprod.timestamp=tstamp;
		newprod.ocupacao=s_espera;
		strcpy(newprod.sala,"sala_espera");
		break;
		case 3: for(int k=0;k<N_LINHAS;k++){
			if(mytmpLines->inicio_medico < tstamp && tstamp <= mytmpLines->fim_medico && mytmpLines->inicio_medico != 9999 && mytmpLines->fim_medico != 9999)s_consulta++;
			mytmpLines++;
		}
		newprod.timestamp=tstamp;
		newprod.ocupacao=s_consulta;
		strcpy(newprod.sala,"sala_consulta");
		break;
	}
	s_admissao=0,s_triagem=0,s_espera=0,s_consulta=0;
	mytmpLines=tmpReset;
	return newprod;
}

PRODUCT produce(void * params,int j){
	long i=(long)params;
	LINE * tempForThread = tmpReset;
	PRODUCT return_prod;
	return_prod=calcular((tmpReset+i),j);
	if(j==N_SALAS){
		params+=PROD;
	}
	return return_prod;
}

void* producer(void * params){
	while(flag_prod<condicao_paragem){
		long increment=(long)
		for(int j=0;j<N_SALAS;j++){
			PRODUCT item=produce(params,j);
			sem_wait(&can_prod);
			pthread_mutex_lock(&mutex_prod);
				buffer[prodptr]=item;
				prodptr=(prodptr+1)%N;
				flag_prod++;
			pthread_mutex_unlock(&mutex_prod);
			sem_post(&can_cons);
		}
	}
	pthread_exit(0);
}

void consume(PRODUCT p){
	char buf[MAX100];
	sprintf(buf,"%ld,%s#%ld\n",p.timestamp,p.sala,p.ocupacao);
	write(destination,buf,strlen(buf));
}

void* consumer(){
	while(flag_cons<condicao_paragem){
		PRODUCT item;
		sem_wait(&can_cons);
		pthread_mutex_lock(&mutex_cons);
			item=buffer[consptr];
			consume(item);
			consptr=(consptr+1)%N;
			flag_prod++;
		pthread_mutex_unlock(&mutex_cons);
		sem_post(&can_prod);
	}
	pthread_exit(0);
}

int main(int argc, char **argv, char **envp){
	char temp[1],msg[MAX100],wc[MAX10],buf[MAX100],filename_str[MAX100],input[MAX100];
	int fds[2],N_ANOS,wcf,pc;
	if (argc != 5){
		perror("Usage : ./program n_producers n_consumers input output");
		exit(-1);
	}
	PROD = atoi(argv[1]);
	CONS = atoi(argv[2]);
	strcpy(input,argv[3]);
	sprintf(filename_str,"wc -l < %s > /tmp/wc.txt",input);
	system(filename_str);
	wcf = open("/tmp/wc.txt",O_RDONLY);
	if(wcf == -1) {
		perror ("Opening WC File");
		exit(-1);
	}
	read(wcf, wc, sizeof(wc));
	N_LINHAS=atoi(wc)-1;				//discard first line
	close(wcf);
	destination = open(argv[4], O_CREAT | O_TRUNC | O_WRONLY, 0666);
	if (destination == -1) {
		perror ("Opening Destination File");
		exit(-1);
	}
	LINE * linhas = (LINE*)calloc(N_LINHAS,sizeof(LINE)); //alocacao de memoria
	FILE *fp=fopen(input,"r");
	if(fp==NULL){
		puts("File Pointer is NULL");
		exit(-1);
	}
	fscanf(fp,"%*s",temp);//apenas ignora a primeira linha do ficheiro
	tmpLines=linhas;
	tmpReset=linhas;
	for(int i=0;i<N_LINHAS;i++){
		fscanf(fp,"%ld %*[;] %ld %*[;] %ld %*[;] %ld %*[;] %ld %*[\n]",&(tmpLines->admissao),&(tmpLines->inicio_triagem),&(tmpLines->fim_triagem),&(tmpLines->inicio_medico),&(tmpLines->fim_medico));
		tmpLines++;
	}
	fclose(fp);
	tmpLines=linhas;
	N_ANOS=number_years(tmpLines,N_LINHAS);
	condicao_paragem=N_LINHAS*N_SALAS;
	flag_cons=0,flag_prod=0;
	sem_init(&can_prod, 0, N);
	sem_init(&can_cons, 0, 0);
	pthread_t consumers[CONS],producers[PROD];
	for(int i=0;i<PROD;i++){
		pthread_create(&producers[i], NULL, producer, (void*)i);
	}
	for(int j=0;j<CONS;j++){
		pthread_create(&consumers[j], NULL, consumer, NULL);
	}
	for(int i=0;i<PROD;i++){
		pthread_join(producers[i], NULL);
	}
	for(int j=0;j<CONS;j++){
		pthread_join(consumers[j], NULL);
	}
	close(destination);
	return 0;
}