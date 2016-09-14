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
#define MAX_BUF 150

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
	//char * cfifoPID;
	int status;
	int len;
	char clientRequest[MAX_BUF];
	char clientBuf[MAX_BUF];
	while(1){
		fprintf(stdout,"$");
		sprintf(clientRequest,"$%d$", getpid());
		fgets(clientBuf, MAX_BUF, stdin);
		len = strlen(clientBuf);
		clientBuf[len-1] = '\0';
		char * IDRequest = concatString(clientRequest,clientBuf);
		//create client FIFO cmdfifo request
		status = mkfifo(cmdfifo,0666);
		clientFd = open(cmdfifo, O_WRONLY);
		write(clientFd, IDRequest, strlen(IDRequest));
		fprintf(stdout,"[%s] is sent to server \n", IDRequest);
		free(IDRequest);
		unlink(cmdfifo);
		close(clientFd);
	 }
	return 0;
}
