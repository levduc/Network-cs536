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
#include <ctype.h>

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
	/*Client Buffer*/ 
	char clientBuf[CLIENT_MAX_BUF];
	/*Client Request*/
	char clientRequest[CLIENT_MAX_BUF];
	int s;
	int len;
	ssize_t numBytesRcvd;
	// char *host; 
	// struct hostent *hp;
	struct sockaddr_in sin;
	/*Client secret key*/ 
	char* c_secret_key; 
    /*filename*/
    char* filename;
	/*Number of byte for single write*/
	int blockSize; 
    char serverBuf[MAX_BUF];
    /*server port*/
	int serverPort;
	/*Build address data structure*/
	/*Check for client input*/
	if(argc != 6)
	{
		printf("Error: <hostname> <portnumber> <secretkey> <filename> <configfile>\n");
		exit(1);
	}
	/*Get IP address*/
  	if(inet_pton(AF_INET, argv[1], &sin.sin_addr)<=0)
    {
        printf("inet_pton error occured\n");
        exit(1);
    }
    /*Get port number*/
    serverPort = strtol(argv[2],NULL,10); 
   	printf("Portnum: %d\n", serverPort);
    /*Check and Get Client Secret Key*/
    if(strlen(argv[3]) < 10)
    {
    	printf("Secret key is too short\n");
	    exit(1);
	}
    if(strlen(argv[3]) > 20){
    	printf("Secret key is too long\n");
        exit(1);
    }
    c_secret_key = argv[3];
    printf("SecretKey: %s\n", c_secret_key);   
    /*Checking file name*/
    //16 character
    if(strlen(argv[4]) > 16)
    {
    	printf("FileName cannot have more than 16 characters\n");
    	exit(1);
    }
    //
    int i;
    for (i = 0; i < strlen(argv[4]); ++i)
    {
    	if (isspace(argv[4][i]))
    	{
    		printf("FileName cannot have a space\n");
    		exit(1);
    	}
    	if ((argv[4][i] == '/'))
    	{
    		printf("FileName cannot have forward slash character\n");
    		exit(1);
    	}
    }
    filename = argv[4];
    printf("FileName: %s\n", filename);
    /*Open Configfile*/
    int fdat; 
    if ((fdat = open(argv[5],O_RDWR)) <= 0)
    {
    	printf("Fail to open file\n");
    	exit(1);
    }
    char bsize[MAX_BUF];
    if(read(fdat, bsize, MAX_BUF) < 0 )
    {
    	printf("Cannot read file\n");
    }
    bsize[MAX_BUF-1] = '\0';
    close(fdat);
    //Convert to int
    blockSize = strtol(bsize, NULL,10);
    printf("BlockSize: %d\n", blockSize);
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
		/*Client Request*/
	    sprintf(clientRequest,"$%s$", c_secret_key);
		//Concatenate with filename
		char * fileRequest = concatString(clientRequest,filename);
		write(s,fileRequest,strlen(fileRequest));
		//free request
		free(fileRequest);
		
		if((numBytesRcvd = read(s, serverBuf, sizeof(serverBuf))) < 0) // recv
    	{
    		perror("read()failed");
    		exit(1);
    	}
    	serverBuf[numBytesRcvd] = '\0';
    	printf("%s\n", serverBuf);
    	close(s);
    	exit(0);
	    memset(clientRequest,0, CLIENT_MAX_BUF);
	    memset(clientBuf,0, CLIENT_MAX_BUF);
	    memset(serverBuf,0, CLIENT_MAX_BUF);
	 }
	return 0;
}