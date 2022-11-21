#include "functions.h"
//Nao esta funcional

void sigusr1();

int main(int argc, char **argv, char **envp){		// Command Line Arguments
	char temp[1],msg[MAX100],wc[MAX10],buf[MAX100],filename_str[MAX1024],input[MAX100];
	int fds[2],N_LINHAS,N_ANOS,destination,wcf,pc;
	pid_t mypid;
	pipe(fds);
	if (argc != 4){
		perror("Usage : ./program n_of_children input output");
		exit(-1);
	}
	int number_pids = atoi(argv[1]);
	int pids[number_pids];
	strcpy(input,argv[2]);
	sprintf(filename_str,"wc -l < %s > /tmp/wc.txt",input);
	system(filename_str);
	wcf = open("/tmp/wc.txt",O_RDONLY);
	if(wcf == -1) {
		perror ("Opening WC File");
		exit(-1);
	}
	read(wcf, wc, sizeof(wc));
	N_LINHAS=atoi(wc)-1;
	close(wcf);
	destination = open(argv[3], O_CREAT | O_TRUNC | O_WRONLY, 0666);
	if (destination == -1) {
		perror ("Opening Destination File");
		exit(-1);
	}
	LINE * linhas = (LINE*)calloc(N_LINHAS,sizeof(LINE)); //alocacao de memoria
	FILE *fp=fopen(input,"r");
	if(fp==NULL){
		puts("erro fp");
		exit(-1);
	}
	fscanf(fp,"%s",temp);//apenas ignora a primeira linha do ficheiro
	LINE * tmpLines=linhas;	//apontador temporario para nao desconfigurar apontador inicial da struct
	LINE * tmpStamp=linhas;	//apontador temporario para nao desconfigurar apontador inicial da struct
	for(int i=0;i<N_LINHAS;i++){
		fscanf(fp,"%ld %*[;] %ld %*[;] %ld %*[;] %ld %*[;] %ld %*[\n]",&(tmpLines->admissao),&(tmpLines->inicio_triagem),&(tmpLines->fim_triagem),&(tmpLines->inicio_medico),&(tmpLines->fim_medico));
		tmpLines++;
	}
	tmpLines=linhas;	//reset no apontador temporario para a struct
	/*CALCULAR NUMERO DE ANOS, CRIAR UM ARRAY COM TAMANHO N_ANOS E POPULA-LO COM OS ANOS*/
	N_ANOS=number_years(tmpLines,N_LINHAS);
	int array_anos[N_ANOS];
	int slots=0,year,flag=0;
	for(int y=0;y<N_LINHAS;y++){
		if(tmpLines->admissao!=9999 && tmpLines->inicio_triagem!=9999 && tmpLines->fim_triagem != 9999 && tmpLines->inicio_medico !=9999 && tmpLines->fim_medico !=9999){
			for(int o=0;o<N_ANOS;o++){
				if((year=return_year_tstamp(tmpLines->admissao))==array_anos[o]){
					flag=1;
					break;
				}
			}
			if(flag==0){
				array_anos[slots]=year;
				slots++;
			}
		}
		flag=0;
		tmpLines++;
	}
	tmpLines=linhas;	//reset no apontador temporario para a struct
	/*LOOP PARA IMPRIMIR TODAS AS OCCORRENCIAS DA STRUCT
	for(int i=0;i<N_LINHAS;i++){
		printf("LINHA:%d ||| %ld %ld %ld %ld %ld\n",i+2,tmpLines->admissao,tmpLines->inicio_triagem,tmpLines->fim_triagem,tmpLines->inicio_medico,tmpLines->fim_medico);
		tmpLines++;
	}*/
	long s_admissao=0,s_triagem=0,s_espera=0,s_consulta=0,timestamp;
	for(int i=0;i<number_pids;i++){ //create child processes
		if ((pids[i]=fork())==-1){
			perror("Fork");
			exit(1);
		}
		if (pids[i] == 0) { 
			close(fds[0]);
			mypid=getpid();
			tmpStamp+=i;
			for(int j=i;j<N_LINHAS-1;j+=number_pids){
				timestamp=tmpStamp->admissao;
				if(timestamp!=9999){ 
				for(int k=0;k<N_LINHAS;k++){
						if(tmpLines->admissao < timestamp && timestamp <= tmpLines->inicio_triagem && tmpLines->admissao != 9999 && tmpLines->inicio_triagem != 9999)s_admissao++;
						if(tmpLines->inicio_triagem < timestamp && timestamp <= tmpLines->fim_triagem && tmpLines->inicio_triagem != 9999 && tmpLines->fim_triagem != 9999)s_triagem++;
						if(tmpLines->fim_triagem < timestamp && timestamp <= tmpLines->inicio_medico && tmpLines->fim_triagem != 9999 && tmpLines->inicio_medico != 9999)s_espera++;
						if(tmpLines->inicio_medico < timestamp && timestamp <= tmpLines->fim_medico && tmpLines->inicio_medico != 9999 && tmpLines->fim_medico != 9999)s_consulta++;
					tmpLines++;
				}
				tmpLines=linhas;
				sprintf(buf,"%d$%d,%ld,espera_triagem#%ld\n",mypid,j,timestamp,s_admissao);
				writen(fds[1],buf,strlen(buf));
				sprintf(buf,"%d$%d,%ld,sala_triagem#%ld\n",mypid,j,timestamp,s_triagem);
				writen(fds[1],buf,strlen(buf));
				sprintf(buf,"%d$%d,%ld,sala_espera#%ld\n",mypid,j,timestamp,s_espera);
				writen(fds[1],buf,strlen(buf));
				sprintf(buf,"%d$%d,%ld,sala_consulta#%ld\n",mypid,j,timestamp,s_consulta);
				writen(fds[1],buf,strlen(buf));
				//pid$id,timestamp,sala#ocupação
				s_admissao=s_triagem=s_espera=s_consulta=0;
				}
				tmpStamp+=number_pids;
			}
			close(fds[1]);
			exit(0);
		}	
	}
	close(fds[1]);
	int pipes_M[N_ANOS][2];//declaração do array de arrays de pipes secundário
	int pids_2[N_ANOS];//declaração do array de pids secundário para os filhos M
	for(int c=0; c<N_ANOS; c++){//inicialização das pipes secundárias
   		pipe(pipes_M[c]);
	}
	for(int i=0;i<N_ANOS;i++){ //criação dos filhos secundários
		if ((pids_2[i]=fork())==-1){
			perror("Fork");
			exit(1);
		}
		signal(SIGUSR1,sigusr1);
		if (pids_2[i] == 0) { 	
			while(readn(pipes_M[i][0],msg,strlen(msg))>0){}	
		}
		close(pipes_M[i][0]);
		for (;;){}
	}
	//variáveis secundárias
	int  linha=0, vars=0;
	long  lotacao_sala;
	char nome_sala[20],msg_2[MAX100],aux[200];
	char *token;
	while(readn(fds[0],msg,strlen(msg))>0){
		//strcpy(msg_2,msg);
		//strcpy(aux,msg);
		strcat(msg_2,msg);
		token =	strtok(msg,"\n");
		while(token!=NULL){
			printf("%s",token);
			if(token[strlen(token)]=='\n'){
				vars=sscanf(token,"%d %*[$] %d %*[,] %ld %*[,] %[^#] %*[#] %d %*[\n]",mypid,linha,timestamp,nome_sala,lotacao_sala);
			}
			sscanf(token,"%d %*[$] %d %*[,] %ld %*[,] %[^#] %*[#] %d %*[\n]",mypid,linha,timestamp,nome_sala,lotacao_sala);
				strcpy(msg_2,token);
				break;
			
			year=return_year_tstamp(timestamp);
			for (int g = 0; g < N_ANOS; g++)
			{
				if(year==array_anos[g]){
					sprintf(buf,"%d$%d,%ld,%s#%ld\n",mypid,linha,timestamp,nome_sala,lotacao_sala);
					writen(pipes_M[g][1],buf,strlen(buf));
				}
			}
			//mypid,linha,timestamp,lotação_sala=0;
			//memset(nome_sala,0,sizeof(nome_sala));
			token =	strtok(msg,"\n");
		}
	}
	for(int c=0; c<N_ANOS; c++){//fechar write das pipes secundárias
   		close(pipes_M[c][1]);
	}
	close(fds[0]);
	for(int l=0;l<number_pids;l++){
		int result;
		waitpid(pids[l],&result,0);
		if(WIFEXITED(result)){
			printf("O processo %d terminou.\n",pids[l]);
		}
	}
	for (int t= 0; t < number_pids; t++)
	{
		kill(pids_2[t],SIGUSR1);
	}
	

	close(destination);
	return 0;
}

void sigusr1(){

	//execlp("./plot.py",,NULL);

}


/*FUNCAO DE CONVERSAO UNIX TIMESTAMP PARA EXTRAIR ANO*/

int return_year_formatted_tstamp(char * msg){
	long r_timestamp;
	sscanf(msg,"%*[^,] %*[,] %ld %*[^\n] %*[\n]",&r_timestamp);
	return return_year_tstamp(r_timestamp);
}

int return_year_tstamp(long timestamp){
	timestamp_timing = *localtime(&timestamp);
	return timestamp_timing.tm_year+1900;
}

/*FUNCOES DE LEITURA E ESCRITA ADICIONAIS (READN & WRITEN)*/


ssize_t             /* Write "n" bytes to a descriptor  */
writen(int fd, const void *ptr, size_t n)
{
	size_t		nleft;
	ssize_t		nwritten;

	nleft = n;
	while (nleft > 0) {
		if ((nwritten = write(fd, ptr, nleft)) < 0) {
			if (nleft == n)
				return(-1); /* error, return -1 */
			else
				break;      /* error, return amount written so far */
		} else if (nwritten == 0) {
			break;
		}
		nleft -= nwritten;
		ptr   += nwritten;
	}
	return(n - nleft);      /* return >= 0 */
}

ssize_t             /* Read "n" bytes from a descriptor  */
readn(int fd, void *ptr, size_t n)
{
	size_t		nleft;
	ssize_t		nread;

	nleft = n;
	while (nleft > 0) {
		if ((nread = read(fd, ptr, nleft)) < 0) {
			if (nleft == n) 	
				return(-1); /* error, return -1 */
			else
				break;      /* error, return amount read so far */
		} else if (nread == 0) {
			break;          /* EOF */
		}
		nleft -= nread;
		ptr   += nread;
	}
	return(n - nleft);      /* return >= 0 */
}

int number_years(LINE * temp, int N_LINHAS){
	LINE * start = temp;
	int year=0,n_years=0,value=0;
	for(int i=0;i<N_LINHAS;i++){
		if(temp->admissao!=9999 && temp->inicio_triagem!=9999 && temp->fim_triagem != 9999 && temp->inicio_medico !=9999 && temp){
			if((value=return_year_tstamp(temp->admissao))!=year){
				n_years++;
				year=value;
			}
		}
		temp++;
	}
	temp=start;
	return n_years;
}