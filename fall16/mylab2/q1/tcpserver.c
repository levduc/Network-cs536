#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define CLIENT_MAX_BUF 1024
#define MAX_BUF 1024
#define SK_MAX 20
int main(int argc, char *argv[])
{
	int status;
    int serverFDPID; // server file description
	char c_secret_key[SK_MAX+1]; // client secret key
	char* s_secret_key; // server secret key
	char command[CLIENT_MAX_BUF+1];
    char cfifopid[CLIENT_MAX_BUF];
	const char d[2] = "$"; //delimtier
  	unsigned int childCount = 0; //child count
  	struct sockaddr_in sin;
    char buf[CLIENT_MAX_BUF];
    int s, new_s; //socket or fd
    int len;
    ssize_t numBytesRcvd;
	int serverPort;
    //checking
    if(argc != 3)
    {
        printf("\n Usage: <portnumber> <secretkey>\n");
        exit(1);
    } 
    
    if(strlen(argv[2]) < 10){
    	printf("Secret key is too short\n");
        exit(1);
    }

    if(strlen(argv[2]) > 20){
    	printf("Secret key is too long\n");
        exit(1);
    }

    //convert to int
    serverPort = strtol(argv[1],NULL,10);
    s_secret_key = argv[2];
    s_secret_key[SK_MAX] = '\0';
    printf("%d\n", serverPort);
    /*build address data structure*/
  	/* Address family = Internet */
    sin.sin_family = AF_INET;
  	/* Set port number, using htons function to use proper byte order */
  	sin.sin_port = htons(serverPort);
  	/* Set IP address to localhost */
  	sin.sin_addr.s_addr = htonl(INADDR_ANY);
  	/* Set all bits of the padding field to 0 */
  	memset(sin.sin_zero, '\0', sizeof sin.sin_zero);
   	
   	if ((s = socket(PF_INET,SOCK_STREAM,0)) < 0) //Check creating 
   	 {
   	 	perror("Fail to create socket");
   	 	exit(1);
   	 } 
   	if ((bind(s, (struct sockaddr *)&sin, sizeof(sin))) < 0)
   	{
   		perror("Fail to bind");
   		exit(1);
   	}
   	listen(s, 10);
	printf("Start listening ... \n");
	while(1) //listening
	{
		memset(buf,0,CLIENT_MAX_BUF);
		if((new_s = accept(s, (struct sockaddr *) &sin, &len)) < 0)
		{
			perror("Fail Accept");
			exit(1);
		}
		printf("Client Requested: \n");
    	if ((numBytesRcvd = read(new_s, buf, sizeof(buf))) < 0) // recv
    	{
    		perror("read fail() failed");
    		exit(1);
    	}
		printf("From client %s\n",buf);
	    pid_t k = fork(); 
		if(k < 0){
			printf("fork() failed!\n");
		}
		else if (k==0) //return 0 when called in the child process
		{ 
		    //fork a child request
			// fork failed
	  		// child code
	    	buf[numBytesRcvd] = '\0';		
			// there is a client request
		    //get the first token
		    char *token;
		    token = strtok(buf, d); 
			//get secret key
			strncpy(c_secret_key, token, SK_MAX+1);
			c_secret_key[SK_MAX] = '\0';
		    token = strtok(NULL, d);
			if(token != NULL)
			{
				strncpy(command, token, CLIENT_MAX_BUF+1);
				command[CLIENT_MAX_BUF] = '\0';
			}
			else
			{
				command[CLIENT_MAX_BUF] = '\0';
			}

			while(token != NULL)
			{
				token = strtok(NULL, d);
			}
	  		// use dup2 stdout to file
		    printf("Comparing secret key\n");
		    dup2(new_s, 1); //stdout
			dup2(new_s, 2); //stderr for error message of execl
		    if(strcmp(s_secret_key, c_secret_key) != 0) //comparing secret key
		    {
		    	printf("Secret key is not matched\n");
		    }
		    else if(execlp(command,command,NULL) == -1)	// if execution failed, terminate child
			{
				perror("execlp");
			   	exit(1);
			}
			//close connection
			close(new_s);
    	}
	  	else if(k>0) 
	  	{	
			// parent code 
		    //terminate child process
	    	close(new_s);   
    		childCount++;
	     	//clean up zombies
	        while (childCount)
	        {
	            k = waitpid((pid_t) - 1, &status, WNOHANG);
	            if (k < 0)
	                perror("waitpid() failed");
	            else if (k == 0)
	                break;
	            else
	                childCount--;
	        }
	  	}
	    memset(c_secret_key,0, CLIENT_MAX_BUF);
	    memset(command,0, CLIENT_MAX_BUF);
	    memset(cfifopid,0, CLIENT_MAX_BUF);
	}
    return 0;
}
