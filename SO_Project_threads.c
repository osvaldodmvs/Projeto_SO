#include "functions.h"

pthread_mutex_t mutex_ocupacoes = PTHREAD_MUTEX_INITIALIZER;
int ocupacoes=0;
int N_THREADS;
int N_LINHAS;
int destination;
LINE * tmpLines;
LINE * tmpReset;

void* work(void* params){
	int i=(long)params;
	char buf[MAX100];
	LINE* mytmpStamp=tmpReset;
	LINE* mytmpLines=tmpLines;
	mytmpStamp+=i;
	long s_admissao=0,s_triagem=0,s_espera=0,s_consulta=0;
	for(int j=i;j<N_LINHAS;j+=N_THREADS){
		int timestamp=mytmpStamp->admissao;
		if(timestamp!=9999){ 
			for(int k=0;k<N_LINHAS;k++){
				if(mytmpLines->admissao < timestamp && timestamp <= mytmpLines->inicio_triagem && mytmpLines->admissao != 9999 && mytmpLines->inicio_triagem != 9999)s_admissao++;
				if(mytmpLines->inicio_triagem < timestamp && timestamp <= mytmpLines->fim_triagem && mytmpLines->inicio_triagem != 9999 && mytmpLines->fim_triagem != 9999)s_triagem++;
				if(mytmpLines->fim_triagem < timestamp && timestamp <= mytmpLines->inicio_medico && mytmpLines->fim_triagem != 9999 && mytmpLines->inicio_medico != 9999)s_espera++;
				if(mytmpLines->inicio_medico < timestamp && timestamp <= mytmpLines->fim_medico && mytmpLines->inicio_medico != 9999 && mytmpLines->fim_medico != 9999)s_consulta++;
				mytmpLines++;
			}
			mytmpLines=tmpReset;
			sprintf(buf,"%ld,espera_triagem#%ld\n",timestamp,s_admissao);
			write(destination,buf,strlen(buf));
			sprintf(buf,"%ld,sala_triagem#%ld\n",timestamp,s_triagem);
			write(destination,buf,strlen(buf));
			sprintf(buf,"%ld,sala_espera#%ld\n",timestamp,s_espera);
			write(destination,buf,strlen(buf));
			sprintf(buf,"%ld,sala_consulta#%ld\n",timestamp,s_consulta);
			write(destination,buf,strlen(buf));
			pthread_mutex_lock(&mutex_ocupacoes);
			ocupacoes++;
			pthread_mutex_unlock(&mutex_ocupacoes);
			s_admissao=s_triagem=s_espera=s_consulta=0;
		}
	}
	pthread_exit(0);
}

int main(int argc, char **argv, char **envp){
	char temp[1],msg[MAX100],wc[MAX10],filename_str[MAX100],input[MAX100];
	int fds[2],wcf,pc;
	int flag=0;
	if (argc != 4){
		perror("Usage : ./program n_workerthreads input output");
		exit(-1);
	}
	N_THREADS = atoi(argv[1]);
	strcpy(input,argv[2]);
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
	destination = open(argv[3], O_CREAT | O_TRUNC | O_WRONLY, 0666);
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
		fscanf(fp,"%ld %*[;] %ld %*[;] %ld %*[;] %ld %*[;] %ld %*[\n]"
			,&(tmpLines->admissao),&(tmpLines->inicio_triagem),&(tmpLines->fim_triagem),&(tmpLines->inicio_medico),&(tmpLines->fim_medico));
		tmpLines++;
	}
	fclose(fp);
	tmpLines=linhas;
	pthread_t worker_threads[N_THREADS];
	for(int i=0;i<N_THREADS;i++){
		pthread_create(&worker_threads[i], NULL, work, (void*)i);
	}
	while(flag<N_LINHAS){
		sleep(1);
		pthread_mutex_lock(&mutex_ocupacoes);
		printf("Ocupacoes calculadas : %d\n",ocupacoes);
		flag=ocupacoes;
		pthread_mutex_unlock(&mutex_ocupacoes);
	}
	for(int i=0;i<N_THREADS;i++){
		pthread_join(worker_threads[i],NULL);
	}
	close(destination);
	return 0;
}