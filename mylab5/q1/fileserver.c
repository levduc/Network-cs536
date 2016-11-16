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

#define CLIENT_MAX_BUF 4096
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
    /*delimiter*/
    const char d[2] = "$"; 
    /*child count*/
    unsigned int childCount = 0; //child count
    /*client Buffer*/
    char buf[CLIENT_MAX_BUF];
    /*Number of bytes received*/
    ssize_t numBytesRcvd;
    /*Socket*/
    int s, new_s; 
    /*Port Number*/
    int serverPort;
    /*Number of byte for single read*/
    int blockSize;
    /*address*/
    struct sockaddr_in sin;
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
    /*create buffer to read from file*/
    char bsize[MAX_BUF];
    memset(bsize, 0, MAX_BUF);
    if(read(fdat, bsize, MAX_BUF) < 0 )
    {
    	printf("Cannot read file\n");
    }
    bsize[MAX_BUF-1] = '\0';
    close(fdat);
    //Convert string to int
    blockSize = strtol(bsize,NULL,10);
    printf("BlockSize: %d\n", blockSize);
    /*Server Port*/
    /*build address data structure*/
    serverPort = strtol(argv[1],NULL,10);
    printf("Server Port: %d\n", serverPort);
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
    //binding
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
        //clear buffer
        memset(buf,0,CLIENT_MAX_BUF);
        /*creating new socket*/
        if((new_s = accept(s, (struct sockaddr *) &sin, &len)) < 0)
        {
            perror("Fail Accept");
	    exit(1);
        }
        /*Wait for request*/
    	if ((numBytesRcvd = read(new_s, buf, sizeof(buf))) < 0)
    	{
    	    perror("read fail() failed");
	    exit(1);
    	}
    	/*Receive a request*/
        printf("Client Request %s\n",buf);
	/*Fork a child to handle client request*/
	pid_t k = fork(); 
	// fork failed		
	if(k < 0)
	{
	   close(new_s);   
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
	    }else
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
	    /*Comparing Secret Key between clent and server*/
	    printf("Comparing Secret Key\n");
	    int fd; //file description
	    if(strcmp(s_secret_key, c_secret_key) != 0) 
	    {
               printf("Keys don't match\n");
               close(new_s);
               exit(1);
	    }
	    /*Key Match. Try open file*/
	    if ((fd = open(fileAddress,O_RDWR)) <= 0)
	    {
               printf("Cannot open file. File may not exist.");
	       close(new_s);
	       exit(1);		    	
	    }
	    /*start writing*/
	    unsigned char writeBuf[blockSize];
	    /*Check if tcp connection is closed by client*/
	    memset(writeBuf, 0, blockSize);
            int byteWrite = 0;
	    while((byteWrite = read(fd, writeBuf, blockSize)) > 0)
	    {
               write(new_s, writeBuf, byteWrite);
	       memset(writeBuf,0, blockSize);
	    }
	    //close connection
	    close(new_s);
	    close(fd);
	    free(fileAddress);
	    memset(fileName,0,CLIENT_MAX_BUF);
	    memset(c_secret_key,0, CLIENT_MAX_BUF);
	    exit(0);
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
}
