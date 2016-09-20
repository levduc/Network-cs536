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
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}

int main(void)
{
	pid_t k;
	int serverFd;
	int clientFd;
	char cmdfifo[50] = "cmdfifo";
	int status;
	int len;
	char clientRequest[CLIENT_MAX_BUF];
	char clientBuf[CLIENT_MAX_BUF];
    char cfifopid[CLIENT_MAX_BUF];
    char serverBuf[MAX_BUF];
	while(1){
		fprintf(stdout,"$");
		int pid = getpid();
		sprintf(clientRequest,"$%d$", pid);
		fgets(clientBuf, CLIENT_MAX_BUF, stdin);
		len = strlen(clientBuf);
		clientBuf[len-1] = '\0';
		char * IDRequest = concatString(clientRequest,clientBuf);

		clientFd = open(cmdfifo, O_RDWR, 0666);
		write(clientFd, IDRequest, strlen(IDRequest));
		fprintf(stdout,"[%s] is sent to server \n", IDRequest);
		printf("============================\n");
		free(IDRequest);
		close(clientFd);
		//create client FIFO cmd cfifoPID
		sprintf(cfifopid,"cfifo%d", pid);
		printf("%s\n", cfifopid);
		if (mkfifo(cfifopid,0666) == -1)
		{
			fprintf(stderr, "Couldn’t create %s FIFO.\n", cfifopid);
        	exit(1);
		}

		serverFd = open(cfifopid, O_RDWR, 0666);
		if(serverFd >= 0)
		{
			//clear weird character
	    	memset(serverBuf,0, CLIENT_MAX_BUF);
	    	//read
		    read(serverFd, serverBuf, MAX_BUF);
			printf("Server Responded: \n"); 
		    fprintf(stdout, "%s \n", serverBuf);
		    close(serverFd);
			//unlink fifo after server responed
			unlink(cfifopid);
			exit(1);
		}
		else{
		    fprintf(stderr, "Error openning to FIFO %s !", cfifopid);
		    exit(1); 
			//fail
		}
		//unlink fifo
		//clear array
	    memset(clientRequest,0, CLIENT_MAX_BUF);
	    memset(clientBuf,0, CLIENT_MAX_BUF);
	    memset(cfifopid,0, CLIENT_MAX_BUF);
	    memset(serverBuf,0, CLIENT_MAX_BUF);
	 }
	unlink(cmdfifo);
	return 0;
}
