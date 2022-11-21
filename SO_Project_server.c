#include "functions.h"

char *socket_path = "/tmp/socket";

int main(int argc, char **argv, char **envp){		// Command Line Arguments
	char wc[MAX10],filename_str[MAX1024],input[MAX100],itoa_position[MAX5],itoa_n_pids[MAX5];
	int N_LINHAS,destination,wcfd;
	if (argc != 4){
		perror("Usage : ./program n_of_children input output");
		exit(-1);
	}
	int number_pids = atoi(argv[1]);
	int pids[number_pids];
	strcpy(input,argv[2]);
	destination=open(argv[3], O_CREAT | O_TRUNC | O_WRONLY, 0666);
	sprintf(itoa_n_pids,"%d",number_pids);
	for(int i=0;i<number_pids;i++){ //create child processes
		if ((pids[i]=fork())==-1){
			perror("Fork");
			exit(1);
		}
		if (pids[i] == 0) {
			//n_filhos,i,input
			sprintf(itoa_position,"%d",i);
			execlp("./so_project_client",itoa_n_pids,itoa_position,input,NULL);
			perror("execlp");
			exit(-1);
		}
	}	
	//variables for the socket
	int listenfd,connfd,fd,bytes;
    char buf[BUF_SIZE];                                                     // buffer for outgoing file
    struct sockaddr_un channel_srv;
    if ( (listenfd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {              // Creating the server socket
    	perror("socket error");
    	exit(-1);
    }
    unlink(socket_path);
    memset(&channel_srv, 0, sizeof(channel_srv));
    channel_srv.sun_family = AF_UNIX;
    strncpy(channel_srv.sun_path, socket_path, sizeof(channel_srv.sun_path)-1);
    if (bind(listenfd, (struct sockaddr*)&channel_srv, sizeof(channel_srv)) == -1) {      // Binding the server socket to its name
        perror("bind error");
        exit(-1);
    }
    if (listen(listenfd, LISTENQ) == -1) {                                  // Configuring the listen queue
        perror("listen error");
        exit(-1);
    }
    	                                                                    // Socket is now set up and bound. Waiting for connections
    for(int l=0;l<number_pids;l++){
        if ((connfd = accept(listenfd, NULL, NULL)) == -1) {
            perror("accept error");
            continue;
        }
        while (1) {
            bytes = readn(connfd, buf, BUF_SIZE);                           // read from socket
            if (bytes <= 0) break;                                          // check for end of information
            writen(destination, buf, bytes);                                // write bytes to file
        }
        close(connfd);                                                      // close connection
    }
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