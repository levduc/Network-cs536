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
#include <arpa/inet.h>

#define CLIENT_MAX_BUF 1024
#define MAX_BUF 1024
#define SK_MAX 20
//used to concatenate two string
char* concatString(char *s1, char *s2)
{
    char *result = malloc(strlen(s1)+strlen(s2)+1);//+1 for the zero-terminator
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}

int main(int argc, char *argv[])
{
	// int serverFd;
	// int clientFd;
	// int status;
	
	// buf
	char clientBuf[CLIENT_MAX_BUF];
	char clientRequest[CLIENT_MAX_BUF];
	int s;
	int len;
	ssize_t numBytesRcvd;
	// char *host; 
	// struct hostent *hp;
	struct sockaddr_in sin;
	
	char* c_secret_key; // client secret key

    char cfifopid[CLIENT_MAX_BUF];
    char serverBuf[MAX_BUF];

	int serverPort;

	/*build address data structure*/
	if(argc != 4)
	{
		printf("Error: <hostname> <portnumber> <secretkey>");
		exit(1);
	}
	else
	{
  		/* Set IP address to localhost */
	  	if(inet_pton(AF_INET, argv[1], &sin.sin_addr)<=0)
	    {
	        printf("\n inet_pton error occured\n");
	        exit(1);
	    }
	    //convert to int
	    serverPort = strtol(argv[2],NULL,10);
	    c_secret_key = argv[3];
	    c_secret_key[SK_MAX] = '\0';
	}
  	/* Address family = Internet */
    sin.sin_family = AF_INET;
  	/* Set port number, using htons function to use proper byte order */
  	sin.sin_port = htons(serverPort);
  	/* Set all bits of the padding field to 0 */
  	memset(sin.sin_zero, '\0', sizeof sin.sin_zero);

	
    //half association
	while(1){
		if((s = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	    {
	        printf("Failed to create socket \n");
	        exit(1);
	    } 
		if(connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
	    {
	       printf("\n Error : Connect Failed \n");
	       close(s);
	       exit(1);
	    } 
		fprintf(stdout,"$");
	    sprintf(clientRequest,"$%s$", c_secret_key);
		fgets(clientBuf, CLIENT_MAX_BUF, stdin);
		len = strlen(clientBuf);
		clientBuf[len-1] = '\0';
		//concatenate
		char * IDRequest = concatString(clientRequest,clientBuf);
		write(s,IDRequest,strlen(IDRequest));
		//free request
		free(IDRequest);
		
		if((numBytesRcvd = read(s, serverBuf, sizeof(serverBuf))) < 0) // recv
    	{
    		perror("read() failed");
    		exit(1);
    	}
    	serverBuf[numBytesRcvd] = '\0';
    	printf("%s\n", serverBuf);
    	close(s);
    	exit(0);
	    memset(clientRequest,0, CLIENT_MAX_BUF);
	    memset(clientBuf,0, CLIENT_MAX_BUF);
	    memset(cfifopid,0, CLIENT_MAX_BUF);
	    memset(serverBuf,0, CLIENT_MAX_BUF);
	 }
	return 0;
}
