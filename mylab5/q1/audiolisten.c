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
#include <time.h>


#define MAX_BUF 1024
#define SK_MAX 20

char * globalBuffer;
/*Number of byte for single write*/
int payloadSize;
int payloadDelay;
int gammaVal;
int bufferSize;  
int targetBufferSize;
int clientUDPSocket;
int currentEndBuffer = 0;
int audioFD;

struct timespec sleepTime, remainTime;

/*concatString is to concatenate two string together*/
char* concatString(char *s1, char *s2)
{
    char *result = malloc(strlen(s1)+strlen(s2)+1); //+1 for the zero-terminator
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}

/*Alarm handler*/
void SIGALARM_handler(int sig_num)
{
    ssize_t numBytesWrt;
    if(currentEndBuffer > 0)
    {
        /*write payloadsize*/
        if(currentEndBuffer >= payloadSize)
        {
            if((numBytesWrt = write(audioFD, globalBuffer, payloadSize)) < 0)
            {
                printf("%s", strerror(errno));
                exit(1);
            }
        }
        else /*otherwise write same as currentEndBuffer*/
        {
            if((numBytesWrt = write(audioFD, globalBuffer, currentEndBuffer)) < 0)
            {
                printf("%s", strerror(errno));
                exit(1);
            }
        }
        // printf("SIGARM: write to file %ld\n", numBytesWrt);
        printf("SIGALARM: current buffer level %d byte written %ld\n",currentEndBuffer, numBytesWrt);
        strcpy(globalBuffer,globalBuffer + numBytesWrt);
        currentEndBuffer = currentEndBuffer - numBytesWrt;
    }
    /*reinstall the handler */
    signal(SIGALRM, SIGALARM_handler); 
}

/*SIGIO handler*/
void SIGIOHandler(int sig_num) 
{
  ssize_t numBytesRcvd;
  do 
  { 
    struct sockaddr_in ssin; //address
    socklen_t sendsize = sizeof(ssin); // Address length in-out parameter
    numBytesRcvd = recvfrom(clientUDPSocket, &globalBuffer[currentEndBuffer], payloadSize, 0, (struct sockaddr *) &ssin, &sendsize);
    if (numBytesRcvd < 0) 
    {
      if (errno != EWOULDBLOCK)
        printf("recvfrom() failed");
    } 
    else 
    {
        // printf("numbytes received %ld\n", numBytesRcvd);
        currentEndBuffer = currentEndBuffer + numBytesRcvd;
        char confirmBack[MAX_BUF];
        sprintf(confirmBack,"Q %d %d %d",currentEndBuffer,targetBufferSize,gammaVal);
        if (sendto(clientUDPSocket,confirmBack,strlen(confirmBack), 0,(struct sockaddr*)&ssin, sizeof(ssin)) < 0)
        {
            printf("Fail to send\n");
            exit(1);
        }
        /*finish sleep*/
        if ((nanosleep(&remainTime,&remainTime)) < 0)
        {
            printf("aslo interupted\n");
        }
    }
  } while (numBytesRcvd > 0);
}

int main(int argc, char *argv[])
{
    /*Client Buffer*/ 
    char clientBuf[MAX_BUF];
    // struct hostent *hp;
    struct sockaddr_in tcp_serversin, udp_csin, udp_ssin;
    /*filename*/
    char * fileName;
    /*logfile name*/
    char * logFileName;
    /*server port*/
    int tcpServerPort, clientUdpPort;
	/*Build address data structure*/
	/*Check for client input*/
	if(argc != 11)
	{
		printf("Usage: <server-ip> <server-tcp-port> <client-udp-port> <payload-size> <playback-del> <gammaVal> <buf-sz> <target-buf> <logfile-s> <filename>\n ");
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
    printf("client udp portnum: %d\n", clientUdpPort);
    
    /*get payload-size*/
    payloadSize = strtol(argv[4], NULL,10);
    printf("Payload size: %d\n", payloadSize);
    
    /*get payload delay*/
    payloadDelay = strtol(argv[5], NULL,10);
    printf("Payload delay: %d\n", payloadDelay);

    /*get gammaVal*/
    gammaVal = strtol(argv[6], NULL,10);
    printf("gammaVal: %d\n", gammaVal);
    
    /*buffer size*/
    bufferSize = strtol(argv[7], NULL,10);
    printf("Buffer size: %d\n", bufferSize);
    //allocate
    globalBuffer = malloc(bufferSize*sizeof(char));
    /*target buffer size*/
    targetBufferSize = strtol(argv[8], NULL,10);
    printf("target buffer size: %d\n", targetBufferSize);
    /*log file name*/
    logFileName = argv[9];
    printf("Log File: %s\n", logFileName);
    
    /*file name*/
    fileName = argv[10];
    printf("file name: %s\n", fileName);

    /***********************tcp server*********************************/
    /* Address family = Internet */
    tcp_serversin.sin_family = AF_INET;
    /* Set port number, using htons function to use proper byte order */
  	tcp_serversin.sin_port = htons(tcpServerPort);
    /* Set all bits of the padding field to 0 */
  	memset(tcp_serversin.sin_zero, '\0', sizeof tcp_serversin.sin_zero);
    /***********************tcp server*********************************/
    /*tcp socket*/
    int tcpSocket;
    ssize_t total;

    /***********************udp client*********************************/
    /* Address family = Internet */
    udp_csin.sin_family = AF_INET;
    /* Set port number, using htons function to use proper byte order */
    udp_csin.sin_port = htons(clientUdpPort);
    /* Set IP address to localhost */
    udp_csin.sin_addr.s_addr = htonl(INADDR_ANY);
    /* Set all bits of the padding field to 0 */
    memset(udp_csin.sin_zero, '\0', sizeof udp_csin.sin_zero);
    if ((clientUDPSocket = socket(AF_INET,SOCK_DGRAM,0)) < 0)
    {
        printf("Fail to create socket");
        exit(1);
    }
    //Binding 
    if ((bind(clientUDPSocket, (struct sockaddr *)&udp_csin, sizeof(udp_csin))) < 0)
    {
        printf("Fail to bind");
        exit(1);
    }
    /***********************udp client*********************************/

    /*Client Request*/
    char clientRequest[MAX_BUF];
    int len;
    const char d[2] = " ";

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
    sprintf(clientRequest,"%d ", clientUdpPort);
	//concatenate with filename
	char * fileRequest = concatString(clientRequest,fileName);
	/*send request to server*/
    write(tcpSocket,fileRequest,strlen(fileRequest));
	//free request
	free(fileRequest);
    /*buffer to be equal block size*/
    char serverBuf[MAX_BUF];
    memset(serverBuf, '\0', MAX_BUF);
    ssize_t numBytesRcvd;
    //file descriptor
    int fd;
    if((numBytesRcvd = read(tcpSocket, serverBuf, MAX_BUF)) < 0)
    {
        printf("error");
        exit(1);
    }
    /*close tcp socket*/
    close(tcpSocket);
    printf("%s\n", serverBuf);
    /*server buffer*/
    serverBuf[numBytesRcvd] = '\0';       
    /*break down server buffer*/
    char *token;
    token = strtok(serverBuf, d); 
    /*getting*/
    char serverAnswer[2];
    strncpy(serverAnswer, token, 2);

    serverAnswer[2] = '\0';
    token = strtok(NULL, d);
    /*client File Name*/
    char serverPortNum[SK_MAX];
    if(token != NULL)
    {
        strncpy(serverPortNum, token, SK_MAX);
        serverPortNum[SK_MAX] = '\0';
    }
    else
    {
        printf("File doesn't exist\n");
        exit(1);
    }

    /***********************udp server*********************************/
    /* Address family = Internet */
    udp_ssin.sin_family = AF_INET;
    /*get server-ip address*/
    if(inet_pton(AF_INET, argv[1], &udp_ssin.sin_addr) <=0)
    {
        printf("inet_pton error occured\n");
        exit(1);
    }
    /* Set port number, using htons function to use proper byte order */
    udp_ssin.sin_port = htons(strtol(serverPortNum,NULL,10));
    /* Set IP address to localhost */
    udp_ssin.sin_addr.s_addr = htonl(INADDR_ANY);
    /* Set all bits of the padding field to 0 */
    memset(udp_ssin.sin_zero, '\0', sizeof udp_ssin.sin_zero);
    /***********************udp server*********************************/


    /***************************SIGIO handler**************************/
    /*SigIO handler*/
    struct sigaction handler;
    handler.sa_handler = SIGIOHandler; // Set signal handler for SIGIO
    // Create mask that mask all signals
    if (sigfillset(&handler.sa_mask) < 0)
      printf("sigfillset() failed");
    handler.sa_flags = 0;              // No flags

    if (sigaction(SIGIO, &handler, 0) < 0)
      printf("sigaction() failed for SIGIO");

    // We must own the socket to receive the SIGIO message
    if (fcntl(clientUDPSocket, F_SETOWN, getpid()) < 0)
      printf("Unable to set process owner to us");

    // Arrange for nonblocking I/O and SIGIO delivery
    if (fcntl(clientUDPSocket, F_SETFL, O_NONBLOCK | FASYNC) < 0)
      printf("Unable to put client sock into non-blocking/async mode");
    /***************************SIGIO handler***************************/
    /*nanosleep*/
    sleepTime.tv_sec = 0;
    sleepTime.tv_nsec = 2500000000; //mili to nano
    if(nanosleep(&sleepTime, &remainTime) < 0)
    {
        printf("Child: nanosleep was interupted %f \n", (remainTime.tv_sec + remainTime.tv_nsec/1000000000.0));
    }

    char * audioFileName = "dcm.mp3";
    audioFD = open(audioFileName, O_CREAT|O_RDWR, 0666);
    if (audioFD < 0) 
    {
        printf("cannot open file: %s \n", audioFileName);
        exit(1);
    }

    int mu = (int) 1000000/gammaVal;
    ualarm(mu,mu);
    signal(SIGALRM, SIGALARM_handler);

    while(1)
    {

    }

	return 0;
}
