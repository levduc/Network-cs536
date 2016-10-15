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

/*concatString is to concatenate two string together*/
char* concatString(char *s1, char *s2)
{
    char *result = malloc(strlen(s1)+strlen(s2)+1); //+1 for the zero-terminator
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}

int main(int argc, char *argv[])
{
	int status;
    int serverFDPID; // server file description
	char * s_secret_key; // server secret key
    char cfifopid[CLIENT_MAX_BUF];
	const char d[2] = "$"; //delimiter
  	unsigned int childCount = 0; //child count
  	struct sockaddr_in sin;
    char buf[CLIENT_MAX_BUF];
    int s, new_s; //socket or fd
    int len;
    ssize_t numBytesRcvd;
	int serverPort;
    /*Number of byte for single read*/
	int blockSize;

    /*Checking user input*/
    if(argc != 4)
    {
        printf("Usage: <portnumber> <secretkey> <configfile>\n");
        exit(1);
    } 
    /*Get and Check Server Secret Key*/
    if(strlen(argv[2]) < 10){
    	printf("Secret key is too short\n");
        exit(1);
    }
    if(strlen(argv[2]) > 20){
    	printf("Secret key is too long\n");
        exit(1);
    }
    s_secret_key = argv[2];
    /*Open Configfile*/
    int fdat; 
    if ((fdat = open(argv[3],O_RDWR)) <= 0)
    {
    	printf("Fail to open file\n");
    	exit(1);
    }

    char bsize[MAX_BUF];
    memset(bsize, 0, MAX_BUF);
    if(read(fdat, bsize, MAX_BUF) < 0 )
    {
    	printf("Cannot read file\n");
    }
    bsize[MAX_BUF-1] = '\0';
    close(fdat);
    //Convert to int
    blockSize = strtol(bsize,NULL,10);
    printf("BlockSize: %d\n", blockSize);

    /*Server Port*/
    serverPort = strtol(argv[1],NULL,10);
    printf("Server Port: %d\n", serverPort);
    /*build address data structure*/
  	/* Address family = Internet */
    sin.sin_family = AF_INET;
  	/* Set port number, using htons function to use proper byte order */
  	sin.sin_port = htons(serverPort);
  	/* Set IP address to localhost */
  	sin.sin_addr.s_addr = htonl(INADDR_ANY);
  	/* Set all bits of the padding field to 0 */
  	memset(sin.sin_zero, '\0', sizeof sin.sin_zero);
  	/*half association*/
   	if ((s = socket(PF_INET,SOCK_STREAM,0)) < 0) 
   	 {
   	 	perror("Fail to create socket");
   	 	exit(1);
   	 } 
   	if ((bind(s, (struct sockaddr *)&sin, sizeof(sin))) < 0)
   	{
   		perror("Fail to bind");
   		exit(1);
   	}
   	/*Listening to incoming request*/
   	listen(s,10);
	printf("Listening ... \n");
	while(1)
	{
		memset(buf,0,CLIENT_MAX_BUF);
		if((new_s = accept(s, (struct sockaddr *) &sin, &len)) < 0)
		{
			perror("Fail Accept");
			exit(1);
		}
    	if ((numBytesRcvd = read(new_s, buf, sizeof(buf))) < 0)
    	{
    		perror("read fail() failed");
    		exit(1);
    	}
		printf("Client Request %s\n",buf);
		/*Fork a child to handle client requests*/
	    pid_t k = fork(); 
		// fork failed		
		if(k < 0)
		{
			printf("fork() failed!\n");
		}
		else if (k==0) //Child Code
		{ 
	    	buf[numBytesRcvd] = '\0';		
		    /*Break down client request*/
		    char *token;
		    token = strtok(buf, d); 
			/*Getting Secret Key*/
			char c_secret_key[SK_MAX+1]; // client secret key
			strncpy(c_secret_key, token, SK_MAX+1);
			c_secret_key[SK_MAX] = '\0';
		    token = strtok(NULL, d);
		    /*Client File Name*/
			char fileName[CLIENT_MAX_BUF+1];
			if(token != NULL)
			{
				strncpy(fileName, token, CLIENT_MAX_BUF+1);
				fileName[CLIENT_MAX_BUF] = '\0';
			}
			else
			{
				fileName[CLIENT_MAX_BUF] = '\0';
			}
			while(token != NULL)
			{
				token = strtok(NULL, d);
			}
			//get filedeposite/fileName
			char *fileLocation = "filedeposit/";
			char * fileAddress = concatString(fileLocation,fileName);
		    
		    printf("Comparing Secret Key\n");
			/*Comparing Secret Key*/
		    int fd; //file description
		    if(strcmp(s_secret_key, c_secret_key) != 0) 
		    {
				printf("Keys don't match\n");
				printf("Cannot open file. File may not exist.");		    	
				close(new_s);
		    	exit(1);
		    }
		    /*Key Match. Try open file*/
		    if ((fd = open(fileAddress,O_RDWR)) <= 0)
		    {
	  			//Use dup2 stdout to file
				printf("Cannot open file. File may not exist.");
				close(new_s);
				exit(1);		    	
		    }
		    /*start writing*/
		    unsigned char writeBuf[blockSize];
		    /*Check if tcp connection is closed by client*/
	    	memset(writeBuf, 0, blockSize);
		    if(read(new_s, writeBuf, blockSize) == 0)
	        {
	            printf("TCP connection is closed by client.\n");
	            exit(1);
	        }
		 	int i = 0;
		 	while((i = read(fd, writeBuf, blockSize)) > 0)
		 	{
		 		write(new_s, writeBuf, i);
	    		memset(writeBuf,0, blockSize);
		 		// printf("%d\n", i);
		 	}
		 	
			//close connection
		    close(fd);
			close(new_s);
	    	free(fileAddress);
	    	// free(fileLocation);
	    	memset(fileName,0,CLIENT_MAX_BUF);
	    	memset(c_secret_key,0, CLIENT_MAX_BUF);
    	}
	  	else if(k>0) 
	  	{	
			// parent code 
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
	}
    return 0;
}
