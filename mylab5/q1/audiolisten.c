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
	char clientBuf[MAX_BUF];
	// struct hostent *hp;
	struct sockaddr_in tcp_serversin, udp_lsin, udp_ssin;
    /*filename*/
    char * fileName;
	/*Number of byte for single write*/
	int payloadSize;
    int payloadDel;
    int gamma;
    int bufferSize;  
    int targetBufferSize;
    /*logfile name*/
    char * logFileName;

    /*server port*/
    int tcpServerPort, clientUdpPort;
	/*Build address data structure*/
	/*Check for client input*/
	if(argc != 11)
	{
		printf("Usage: <server-ip> <server-tcp-port> <client-udp-port> <payload-size> <playback-del> <gamma> <buf-sz> <target-buf> <logfile-s> <filename>\n ");
		exit(1);
	}

	/*get server-ip address*/
  	if(inet_pton(AF_INET, argv[1], &tcp_serversin.sin_addr)<=0)
    {
        printf("inet_pton error occured\n");
        exit(1);
    }

    /*get server-tcp-port number*/
    tcpServerPort = strtol(argv[2],NULL,10); 
   	printf("Server tcp portnum: %d\n", tcpServerPort);
    
    /*get client-udp-portnumber*/
    clientUdpPort = strtol(argv[3],NULL,10); 
    printf("Server tcp portnum: %d\n", clientUdpPort);
    
    /*get payload-size*/
    payloadSize = strtol(argv[4], NULL,10);
    printf("Payload size: %d\n", payloadSize);
    
    /*get payload-del*/
    payloadDel = strtol(argv[5], NULL,10);
    printf("Payload del: %d\n", payloadDel);

    /*get gamma*/
    gamma = strtol(argv[6], NULL,10);
    printf("Gamma: %d\n", gamma);
    
    /*buffer size*/
    bufferSize = strtol(argv[7], NULL,10);
    printf("Buffer size: %d\n", bufferSize);

    /*target buffer size*/
    targetBufferSize = strtol(argv[8], NULL,10);
    printf("Buffer Size: %d\n", targetBufferSize);
    /*log file name*/
    logFileName = argv[9];
    printf("Log File: %s\n", logFileName);
    
    /*file name*/
    fileName = argv[10];
    printf("file name: %s\n", fileName);
    
    /* Address family = Internet */
    tcp_serversin.sin_family = AF_INET;
  	
    /* Set port number, using htons function to use proper byte order */
  	tcp_serversin.sin_port = htons(tcpServerPort);
  	
    /* Set all bits of the padding field to 0 */
  	memset(tcp_serversin.sin_zero, '\0', sizeof tcp_serversin.sin_zero);
    
    /*tcp socket*/
    int tcpSocket;
    ssize_t total;
    /*Client Request*/
    char clientRequest[MAX_BUF];
    int len;

    while(1)
    {
		if((tcpSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	    {
	        printf("Failed to create socket \n");
	        exit(1);
	    } 
		if(connect(tcpSocket, (struct sockaddr *)&tcp_serversin, sizeof(tcp_serversin)) < 0)
	    {
	       printf("\n Error : Connect Failed \n");
	       close(tcpSocket);
	       exit(1);
	    } 
		/*client request*/
	    sprintf(clientRequest,"$%d$", clientUdpPort);
		//concatenate with filename
		char * fileRequest = concatString(clientRequest,fileName);
		/*send request to server*/
        write(tcpSocket,fileRequest,strlen(fileRequest));
		//free request
		free(fileRequest);
        /*buffer to be equal block size*/
        unsigned char serverBuf[payloadSize];
        memset(serverBuf, '\0', payloadSize);
        ssize_t numBytesRcvd;
        //file descriptor
        int fd;
        if( (numBytesRcvd = read(tcpSocket, serverBuf, payloadSize)) < 0)
        {
            printf("error");
            exit(1);
        }
        printf("%s\n", serverBuf);
        // //measure Time
        // struct timeval start, end;
        // int firstRead = 1;
        // /*Wait for resspone from server*/
        // while ((numBytesRcvd = read(s, serverBuf, payloadSize)) > 0) // recv
        // {
        //     /*Check for first read*/
        //     if(firstRead == 1)
        //     {
        //         /*Creating file*/
        //         fd = open(fileName, O_CREAT | O_WRONLY | O_EXCL, S_IRUSR | S_IWUSR);
        //         if (fd < 0) 
        //         {
        //           /*failure: file existed */
        //           if (errno == EEXIST) {
        //             printf("File already existed\n");
        //             close(s);
        //             exit(1);
        //           }
        //         }
        //         //Start Time 
        //         gettimeofday(&start, NULL);
        //         firstRead = 0;
        //     }
        //     write(fd,serverBuf,numBytesRcvd);
        //     memset(serverBuf, '\0', payloadSize);
        //     total += numBytesRcvd;
        // }
        // /*numBytesRcvd = 0 implies TCP connection is closed*/
        // if(firstRead == 1) 
        // {
        //     printf("TCP connection is closed. Key don't match or File may not exist\n");
        //     exit(1);            
        // }
        // //End Time 
        // gettimeofday(&end, NULL);
        // //numBytes = 0 implies tcp connection closed
        // fprintf(stdout,"TCP connection is closed\n");
    	exit(0);
	 }
	return 0;
}
