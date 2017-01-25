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
	char *s = "$ "; //delimiter
	char *argv[20]; //argument
	int status;
	if (mkfifo(cmdfifo,0666) == -1){
		unlink(cmdfifo);
		if (mkfifo(cmdfifo,0666) == -1){
			//
		}
	}
	while(1){
		//create client FIFO cmdfifo request
		//open client request
		clientRequestFD = open(cmdfifo, O_RDONLY,0666);
		// there is a client request
		if(clientRequestFD >= 0) 
		{
		    memset(buf,0, CLIENT_MAX_BUF);
			//read client request
		    read(clientRequestFD, buf, CLIENT_MAX_BUF);
		    close(clientRequestFD);
		    //delimiter
		    //get the first token
		    char *token;
		    token = strtok(buf, s); 
			//get client id
			strncpy(pid, token, CLIENT_MAX_BUF+1);
			pid[CLIENT_MAX_BUF] = '\0';
			int i = 0;
			token = strtok(NULL, s);
			if(token == NULL){
				argv[i] = " ";
				i++;
			}
			while(token != NULL)
			{
		    	argv[i] = token;
		    	printf("%s\n", argv[i]);
				token = strtok(NULL, s);
				i++;
			}
			argv[i] = NULL;
		    printf("Received Request From: %s\n", pid);
			//clear buffer
		    //open client cfifoPID
			sprintf(cfifopid,"cfifo%s", pid);
			//try open with write permission
			int count = 0;
			while(count <100) //reopen multiple time
			{
				serverFDPID = open(cfifopid, O_RDWR, 0666);
				if(serverFDPID >= 0) //success
				{
				    count = 101; //escape while loop
				    //fork a child request
				    k = fork(); 
					if(k < 0){
						// fork failed
        				printf("fork() failed!\n");
					}
					if (k==0) //return 0 when called in the child process
					{ 
				  		// child code
				  		// use dup2 stdout to file
						dup2(serverFDPID, 1); //stdout
						dup2(serverFDPID, 2); //stderr for error message of execl
					    execvp(argv[0], argv);// if execution failed, terminate child
						perror("execvp");
						exit(1);
					}
				  	if(k>0) 
				  	{	
						// parent code 
					    //terminate child process
				     	int w = waitpid(k, &status, WNOHANG);
				     	if(w == -1) {
				     		//fail
				     		printf("wait fails\n");
						} 
						else {
						 
						}
				  	}
				    printf("Server Responded To: %s\n", pid);
				} 
				else {
					//try reopen
				}
				count++;
			}
		    memset(buf,0, CLIENT_MAX_BUF);
		    memset(pid,0, CLIENT_MAX_BUF);
		    memset(cfifopid,0, CLIENT_MAX_BUF);
		}
	}
    return 0;
}