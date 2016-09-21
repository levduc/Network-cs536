#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define CLIENT_MAX_BUF 150


int main()
{
    int clientRequestFD;
    int serverFDPID;
    char cmdfifo[50] = "cmdfifo";
    pid_t k;
    /* open, read, and display the message from the FIFO */
    char buf[CLIENT_MAX_BUF];
	char pid[CLIENT_MAX_BUF+1];
	char command[CLIENT_MAX_BUF+1];
    char cfifopid[CLIENT_MAX_BUF];
	const char s[2] = "$";
	int status;
	while(1){
		//open client request
		clientRequestFD = open(cmdfifo, O_RDONLY,0666);
		// there is a client request
		if(clientRequestFD >= 0) 
		{
		    memset(buf,0, CLIENT_MAX_BUF);
			//read client request
		    read(clientRequestFD, buf, CLIENT_MAX_BUF);
		    close(clientRequestFD);
		    unlink(cmdfifo);
		    //delimiter
		    //get the first token
		    char *token;
		    token = strtok(buf, s); 
			//get client id
			strncpy(pid, token, CLIENT_MAX_BUF+1);
			pid[CLIENT_MAX_BUF] = '\0';
		    token = strtok(NULL, s);
			strncpy(command, token, CLIENT_MAX_BUF)+1;
			command[CLIENT_MAX_BUF] = '\0';
			while(token != NULL)
			{
				token = strtok(NULL, s);
			}
		    printf("Received Request From: %s\n", pid);
			//clear buffer
		    memset(buf,0, CLIENT_MAX_BUF);
		    //open client cfifoPID
			sprintf(cfifopid,"cfifo%s", pid);
			printf("%s\n", cfifopid);
			//try open with write permission
			int count = 0;
			while(count <10)
			{
				serverFDPID = open(cfifopid, O_RDWR, 0666);
				if(serverFDPID >= 0) //success
				{
				    count = 11; 
				    //fork a child request
				    k = fork(); 
					if (k==0) //return 0 when called in the child process
					{ 
				  		// child code
				  		// use dup2 stdout to file
						dup2(serverFDPID, 1); //stdout
						dup2(serverFDPID, 2); //stderr for error message of execl
					    if(execlp(command,command,NULL) == -1)	// if execution failed, terminate child
						{
							perror("execlp");
						   	exit(1);
						}
				    	close(serverFDPID);   
					}
				  	else 
				  	{	
						// parent code 
					    //terminate child process
				    	close(serverFDPID);
				     	if(waitpid(k, &status, WNOHANG) != 0) {
				     		//
						} 
						else {
						 
						}
				  	}
				    printf("Server Responded To: %s\n", pid);
				} else {
					fprintf(stderr, "Error openning to FIFO %s ! Trying to open again\n", cfifopid);
					//try reopen
				}
				count++;
			}

		    memset(pid,0, CLIENT_MAX_BUF);
		    memset(command,0, CLIENT_MAX_BUF);
		    memset(cfifopid,0, CLIENT_MAX_BUF);
		}
	}
    return 0;
}