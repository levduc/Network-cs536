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


int main()
{
    int clientRequestDF;
    char * cmdfifo = "cmdfifo";
    /* open, read, and display the message from the FIFO */
    char buf[CLIENT_MAX_BUF];
	char pid[31];
	char command[31];
	while(1){
		clientRequestDF = open(cmdfifo, O_RDONLY);
		if(clientRequestDF > 0) // there is request
		{
		    read(clientRequestDF, buf, CLIENT_MAX_BUF);
		    int lens = strlen(buf);
		    printf("%d\n", lens);		    
		    //delimiter
		    const char s[2] = "$";
		    //get the first token
		    char *token;
		    token = strtok(buf, s); 
			strncpy(pid, token, 30);
			pid[30] = '\0';
		    token = strtok(NULL, s);
			strncpy(command, token, 30);
			pid[30] = '\0';
			while(token != NULL)
			{
				token = strtok(NULL, s);
			}
		    printf("Received From: %s\n", buf);
		    printf("Received From: %s\n", pid);
		    printf("Received From: %s\n", command);
		    close(clientRequestDF);
		    memset(buf,0, CLIENT_MAX_BUF);
		    memset(pid,0, CLIENT_MAX_BUF);
		    memset(command,0, CLIENT_MAX_BUF);
		}
		else{
			continue;
		}
	}
    //return 0;
}