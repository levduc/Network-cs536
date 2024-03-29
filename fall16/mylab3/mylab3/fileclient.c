#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <errno.h>
#include <fcntl.h>
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
	/*Client Buffer*/ 
	char clientBuf[CLIENT_MAX_BUF];
	/*Client Request*/
	char clientRequest[CLIENT_MAX_BUF];
	int s;
	int len;
	// struct hostent *hp;
	struct sockaddr_in sin;
	/*Client secret key*/ 
	char* c_secret_key; 
    /*filename*/
    char* fileName;
	/*Number of byte for single write*/
	int blockSize; 
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
    //Checking for spacing and forward slash
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
    fileName = argv[4];
    printf("FileName: %s\n", fileName);
    /*Open Configfile*/
    int fdat; 
    if ((fdat = open(argv[5],O_RDWR)) <= 0)
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
    //Convert String to Int
    blockSize = strtol(bsize, NULL,10);
    printf("BlockSize: %d\n", blockSize);
  	/* Address family = Internet */
    sin.sin_family = AF_INET;
  	/* Set port number, using htons function to use proper byte order */
  	sin.sin_port = htons(serverPort);
  	/* Set all bits of the padding field to 0 */
  	memset(sin.sin_zero, '\0', sizeof sin.sin_zero);
    //half association
	ssize_t total;
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
		/*Client Request*/
	    sprintf(clientRequest,"$%s$", c_secret_key);
		//Concatenate with filename
		char * fileRequest = concatString(clientRequest,fileName);
		write(s,fileRequest,strlen(fileRequest));
		//free request
		free(fileRequest);
        /*Buffer to be equal block size*/
        unsigned char serverBuf[blockSize];
        memset(serverBuf, '\0', blockSize);
        ssize_t numBytesRcvd;
        //file descriptor
        int fd;
        //total bytes
        total = 0;
        //Measure Time
        struct timeval start, end;
        int firstRead = 1;
        /*Wait for resspone from server*/
        while ((numBytesRcvd = read(s, serverBuf, blockSize)) > 0) // recv
        {
            /*Check for first read*/
            if(firstRead == 1)
            {
                /*Creating file*/
                fd = open(fileName, O_CREAT | O_WRONLY | O_EXCL, S_IRUSR | S_IWUSR);
                if (fd < 0) 
                {
                  /*failure: file existed */
                  if (errno == EEXIST) {
                    printf("File already existed\n");
                    close(s);
                    exit(1);
                  }
                }
                //Start Time 
                gettimeofday(&start, NULL);
                firstRead = 0;
            }
            write(fd,serverBuf,numBytesRcvd);
            memset(serverBuf, '\0', blockSize);
            total += numBytesRcvd;
        }
        /*numBytesRcvd = 0 implies TCP connection is closed*/
        if(firstRead == 1) 
        {
            printf("TCP connection is closed. Key don't match or File may not exist\n");
            exit(1);            
        }
        //End Time 
        gettimeofday(&end, NULL);
        //numBytes = 0 implies tcp connection closed
        fprintf(stdout,"TCP connection is closed\n");
    	//Close socket
        close(s);
        /*Printing information*/
    	fprintf(stdout,"=======================================\n");
    	fprintf(stdout,"\"%s\" is downloaded.\n", fileName);
    	fprintf(stdout,"Number of Bytes: %ld bytes\n", total);
        /*Calculate completion time in ms*/
        float t = (end.tv_sec - start.tv_sec)*1000.0 + (end.tv_usec - start.tv_usec)/1000.0;
    	fprintf(stdout,"Completion Time: %f ms\n", t);
    	fprintf(stdout,"Reliable Throughput: %f bps\n", (total*8*1000)/t);
    	exit(0);
	 }
	return 0;
}
