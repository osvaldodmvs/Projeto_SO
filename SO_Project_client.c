#include "functions.h"

char *socket_path = "/tmp/socket";

int main(int argc, char **argv, char **envp){		// Command Line Arguments
	char temp[1],wc[MAX10],filename_str[MAX1024],input[MAX100];
	int N_LINHAS,N_LINHAS_STRUCT,wcf;
	int number_pids = atoi(argv[0]);
	int position = atoi(argv[1]);
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
	N_LINHAS_STRUCT=0;
	if((N_LINHAS%number_pids)!=0)
		N_LINHAS_STRUCT=1;
	N_LINHAS_STRUCT+=(N_LINHAS/number_pids);
	close(wcf);
	LINE * linhas = (LINE*)calloc(N_LINHAS_STRUCT,sizeof(LINE)); //alocacao de memoria
	FILE *fp=fopen(input,"r");
	if(fp==NULL){
		puts("erro fp");
		exit(-1);
	}
	fscanf(fp,"%*s",temp);//apenas ignora a primeira linha do ficheiro
	for(int i=0;i<position;i++){	//salta n linhas inicialmente para comecar a ler na linha correta
		fscanf(fp,"%*s",temp);
	}
	LINE * tmpLines=linhas;	//apontador temporario para nao desconfigurar apontador inicial da struct
	LINE * tmpStamp=linhas;	//apontador temporario para nao desconfigurar apontador inicial da struct
	for(int i=0;i<N_LINHAS_STRUCT;i++){
		fscanf(fp,"%ld %*[;] %ld %*[;] %ld %*[;] %ld %*[;] %ld %*[\n]",&(tmpLines->admissao),&(tmpLines->inicio_triagem),&(tmpLines->fim_triagem),&(tmpLines->inicio_medico),&(tmpLines->fim_medico));
		for(int j=0;j<number_pids-1;j++){
			fscanf(fp,"%*s",temp);
		}
		tmpLines++;
	}

	/*
	STRUCT DIVIDIDA POR TODOS OS FILHOS CRIA PROBLEMA DE COMPARACOES, MEMORIA TOTAL E A MESMA QUE UMA STRUCT NO PAI MAS OS FILHOS SO FAZEM (N_LINHAS/N_FILHOS) COMPARACOES
	*/

	fclose(fp);
	tmpLines=linhas;
	//variables for the socket
	int c, uds, bytes;
    char buf[BUF_SIZE]; /* buffer for incoming file */
    struct sockaddr_un channel; /* Unix Domain socket */

    if ( (uds = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket error");
        exit(1);
    }
    
    memset(&channel, 0, sizeof(channel));
    channel.sun_family= AF_UNIX;
    strncpy(channel.sun_path, socket_path, sizeof(channel.sun_path)-1);
    
    if (connect(uds, (struct sockaddr*)&channel, sizeof(channel)) == -1) {
        perror("connect error");
        exit(1);
    }
    long s_admissao=0,s_triagem=0,s_espera=0,s_consulta=0,timestamp;
    pid_t mypid=getpid();
    for(int j=0;j<N_LINHAS_STRUCT;j++){
    	timestamp=tmpStamp->admissao;
    	if(timestamp!=9999){ 
    		for(int k=0;k<N_LINHAS_STRUCT;k++){
    			if(tmpLines->admissao < timestamp && timestamp <= tmpLines->inicio_triagem && tmpLines->admissao != 9999 && tmpLines->inicio_triagem != 9999)s_admissao++;
    			if(tmpLines->inicio_triagem < timestamp && timestamp <= tmpLines->fim_triagem && tmpLines->inicio_triagem != 9999 && tmpLines->fim_triagem != 9999)s_triagem++;
    			if(tmpLines->fim_triagem < timestamp && timestamp <= tmpLines->inicio_medico && tmpLines->fim_triagem != 9999 && tmpLines->inicio_medico != 9999)s_espera++;
    			if(tmpLines->inicio_medico < timestamp && timestamp <= tmpLines->fim_medico && tmpLines->inicio_medico != 9999 && tmpLines->fim_medico != 9999)s_consulta++;
    			tmpLines++;
    		}
    		tmpLines=linhas;
    		sprintf(buf,"%d$%d,%ld,espera_triagem#%ld\n",mypid,j,timestamp,s_admissao);
    		writen(uds, buf, strlen(buf));                          
    		sprintf(buf,"%d$%d,%ld,sala_triagem#%ld\n",mypid,j,timestamp,s_triagem);
    		writen(uds, buf,strlen(buf));  
    		sprintf(buf,"%d$%d,%ld,sala_espera#%ld\n",mypid,j,timestamp,s_espera);
    		writen(uds, buf,strlen(buf));  
    		sprintf(buf,"%d$%d,%ld,sala_consulta#%ld\n",mypid,j,timestamp,s_consulta);
    		writen(uds, buf,strlen(buf));  
        	s_admissao=s_triagem=s_espera=s_consulta=0;
    	}
    	tmpStamp++;
    }
    close(uds);
exit(0);
}


/*FUNCOES DE LEITURA E ESCRITA ADICIONAIS (READN & WRITEN)*/


ssize_t             /* Write "n" bytes to a descriptor  */
writen(int fd, const void *ptr, size_t n){
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
	readn(int fd, void *ptr, size_t n){
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