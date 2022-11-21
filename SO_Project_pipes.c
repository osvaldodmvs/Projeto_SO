#include "functions.h"

int main(int argc, char **argv, char **envp){		// Command Line Arguments
	char temp[1],msg[MAX100],wc[MAX10],buf[MAX100],filename_str[MAX1024],input[MAX100];
	int fds[2],N_LINHAS,destination,wcf,pc;
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
	fscanf(fp,"%*s",temp);//apenas ignora a primeira linha do ficheiro
	LINE * tmpLines=linhas;	//apontador temporario para nao desconfigurar apontador inicial da struct
	LINE * tmpStamp=linhas;	//apontador temporario para nao desconfigurar apontador inicial da struct
	for(int i=0;i<N_LINHAS;i++){
		fscanf(fp,"%ld %*[;] %ld %*[;] %ld %*[;] %ld %*[;] %ld %*[\n]",&(tmpLines->admissao),&(tmpLines->inicio_triagem),&(tmpLines->fim_triagem),&(tmpLines->inicio_medico),&(tmpLines->fim_medico));
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
			exit(-1);
		}
		if (pids[i] == 0) { 
			close(fds[0]);
			pid_t mypid=getpid();
			tmpStamp+=i;
			for(int j=i;j<N_LINHAS;j+=number_pids){
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
	while(pc=readn(fds[0],msg,strlen(msg))>0){
		writen(destination,msg,strlen(msg));
	}
	close(fds[0]);
	for(int l=0;l<number_pids;l++){
		int result;
		waitpid(pids[l],&result,0);
		if(WIFEXITED(result)){
			printf("O processo %d terminou.\n",pids[l]);
		}
	}
	close(destination);
	return 0;
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
