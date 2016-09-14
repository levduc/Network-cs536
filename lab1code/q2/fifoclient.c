// simple shell example using fork() and execlp()
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <stdlib.h>

char* concatenation(char *s1, char *s2)
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
	int serverFd, clientFd;
	char * cmdfifo = "cmdfifo";
	//get client pid
	//get client command
	int status;
	int len;
	char clientRequest[100];
	char clientBuf[100];
	while(1){
		fprintf(stdout,"[%d]$ ",getpid());
		sprintf(clientRequest,"$%d$", getpid());
		fgets(clientBuf, 100, stdin);
		len = strlen(clientBuf);
		clientBuf[len-1] = '\0';
		char * IDRequest = concatenation(clientRequest,clientBuf);
		fprintf(stderr, "%s\n", IDRequest);
		clientFd = open(cmdfifo, IDRequest, sizeof(IDRequest));
		free(IDRequest);
		unlink(fifo);
	}
}
