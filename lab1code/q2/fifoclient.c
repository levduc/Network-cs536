// simple shell example using fork() and execlp()
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdlib.h>
#define CLIENT_MAX_BUF 150
#define MAX_BUF 1024

char* concatString(char *s1, char *s2)
{
    char *result = malloc(strlen(s1)+strlen(s2)+1);//+1 for the zero-terminator
    //in real code you would check for errors in malloc here
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}

int main(void)
{
	pid_t k;
	int serverFd;
	int clientFd;
	char * cmdfifo = "cmdfifo";
	int status;
	int len;
	char clientRequest[CLIENT_MAX_BUF];
	char clientBuf[CLIENT_MAX_BUF];
    char buf[MAX_BUF];
	while(1){
		fprintf(stdout,"$");
		int pid = getpid();
		sprintf(clientRequest,"$%d$", pid);
		fgets(clientBuf, CLIENT_MAX_BUF, stdin);
		len = strlen(clientBuf);
		clientBuf[len-1] = '\0';
		char * IDRequest = concatString(clientRequest,clientBuf);
		//create client FIFO cmdfifo request
		//status = mkfifo(cmdfifo,0666);
		mkfifo(cmdfifo,0666);
		clientFd = open(cmdfifo, O_WRONLY);
		write(clientFd, IDRequest, strlen(IDRequest));
		fprintf(stdout,"[%s] is sent to server \n", IDRequest);
		free(IDRequest);
		// unlink(cmdfifo);
		close(clientFd);
		//create client FIFO cmd cfifoPID
		// char cfifopid[CLIENT_MAX_BUF];
		// sprintf(cfifopid,"cfifo%d", pid);
		// printf("%s\n", cfifopid);
		// mkfifo(cfifopid,0666);
		// serverFd = open(cfifopid, O_RDONLY);
		// if(fd >= 0)
		// {
		//     read(serverFd, buf, MAX_BUF);
		//     printf("Server Response: %s\n", buf);
		//     close(serverFd);
		// }
		// else{

		// 	//fail
		// }
	 }
	return 0;
}
