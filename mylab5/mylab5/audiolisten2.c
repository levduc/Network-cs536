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
#include <semaphore.h>
#include <pthread.h>
#define MAX_BUF 1000000
#define EXMAX_BUF 1000000000
#define SK_MAX 20

char * globalBuffer;
/*Number of byte for single write*/
int payloadSize;
float payloadDelay;
float gammaVal;
int bufferSize;  
int discardK;
int targetBufferSize;
int clientUDPSocket;
int currentEndBuffer = 0;
int audioFD;
int retransmitFlag;
/*define semaphore*/
sem_t smp;
sem_t waitForCurrentEndBuffer;
/*Log Buffer*/
char LogBuffer[EXMAX_BUF];
struct timeval start, end;
int firstRead = 0;
int endTranmission = 0;
struct timespec sleepTime, remainTime;
int packageCount=0;
int lastPacket=0;
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
            while((numBytesWrt = write(audioFD, globalBuffer, payloadSize)) < 0)
            {
            }
        }
        else /*otherwise write same as currentEndBuffer*/
        {
            while((numBytesWrt = write(audioFD, globalBuffer, currentEndBuffer)) < 0)
            {
            }
        }
        // printf("SIGARM: write to file %ld\n", numBytesWrt);
        if(numBytesWrt > 0)
        {
            // printf("SIGALARM: current buffer level %d byte written %ld\n", currentEndBuffer, numBytesWrt);
            strcpy(globalBuffer,globalBuffer + numBytesWrt);
            sem_wait(&smp);
            currentEndBuffer = currentEndBuffer - numBytesWrt;
            sem_post(&smp);
        }
    }
    /*reinstall the handler */
    signal(SIGALRM, SIGALARM_handler);
}


/*SIGIO handler*/
void SIGIOHandler(int sig_num) 
{
  /*increase number of package received*/
  packageCount++;
  if(firstRead == 0)
  {
    gettimeofday(&start, NULL);
    firstRead = 1;
  }
  if(packageCount%discardK == 0) /*ignore*/
  {
      struct sockaddr_in ssin; //address
      socklen_t sendsize = sizeof(ssin); // Address length in-out parameter
      char tempBuf[MAX_BUF];
      int dcm = recvfrom(clientUDPSocket, tempBuf, payloadSize+4, 0, (struct sockaddr *) &ssin, &sendsize);
      if(dcm != 3)
      {
  
            printf("Discard %x%x%x%x\n", (unsigned char)tempBuf[0],(unsigned char)tempBuf[1],(unsigned char)tempBuf[2],(unsigned char)tempBuf[3]);  
      }
  }
  else /*do work*/
  {     
      if(currentEndBuffer + payloadSize < bufferSize)
      {
          ssize_t numBytesRcvd;
          do 
          { 
            struct sockaddr_in ssin; //address
            socklen_t sendsize = sizeof(ssin); // Address length in-out parameter
            char * temp = malloc(payloadSize+4);
            // memset(temp,0,payloadSize+4);
            numBytesRcvd = recvfrom(clientUDPSocket, &globalBuffer[currentEndBuffer], payloadSize+4, 0, (struct sockaddr *) &ssin, &sendsize);
            if (numBytesRcvd < 0) 
            {
              if (errno != EWOULDBLOCK)
              {
                    printf("1: recvfrom() failed");
                    // exit(1);
              }
            } 
            else if(numBytesRcvd != 3) 
            {
                endTranmission =0;
                int isGood = 0;
                int temp =  (int)((unsigned char)(globalBuffer[currentEndBuffer]) << 24 |
                                (unsigned char)(globalBuffer[currentEndBuffer + 1]) << 16 |
                                (unsigned char)(globalBuffer[currentEndBuffer + 2]) << 8 |
                                (unsigned char)(globalBuffer[currentEndBuffer + 3]));
                /*checking packet*/
                if(temp < lastPacket) /*ignore*/
                {
                    isGood = 0;
                    printf("ignore %d\n", temp);
                } 
                else if(temp > lastPacket+1) /*detect missing*/
                {
                    isGood = 1;
                    lastPacket = temp; /*updating*/
                    if(retransmitFlag == 1)
                    {
                        unsigned char negACK[6];
                        negACK[0] = 'M';
                        negACK[1] = ' ';
                        negACK[2] = ((lastPacket) >> 24) & 0xFF;
                        negACK[3] = ((lastPacket) >> 16) & 0xFF;
                        negACK[4] = ((lastPacket) >> 8) & 0xFF;
                        negACK[5] = (lastPacket) & 0xFF;
                        if (sendto(clientUDPSocket,negACK,6, 0,(struct sockaddr*)&ssin, sizeof(ssin)) < 0)
                        {
                            printf("Fail to send\n");
                            exit(1);
                        }
                        printf("negACK sent: %c %x%x%x%x\n", negACK[0], negACK[2],negACK[3],negACK[4],negACK[5]);
                    }
                } 
                else if (temp == lastPacket +1)
                {
                    isGood=1;
                    lastPacket = temp;
                }
                if(isGood)
                {
                    memmove(globalBuffer+currentEndBuffer, globalBuffer+currentEndBuffer+4, numBytesRcvd-4);
                    // strcpy(globalBuffer+currentEndBuffer,temp+4);
                    sem_wait (&smp);
                    currentEndBuffer = currentEndBuffer + numBytesRcvd;
                    sem_post (&smp);
                    char confirmBack[MAX_BUF];
                    sprintf(confirmBack,"Q %d %d %f",currentEndBuffer,targetBufferSize,gammaVal);
                    if (sendto(clientUDPSocket,confirmBack,strlen(confirmBack), 0,(struct sockaddr*)&ssin, sizeof(ssin)) < 0)
                    {
                        printf("Fail to send\n");
                        exit(1);
                    }
                    gettimeofday(&end, NULL);
                    sprintf(LogBuffer + strlen(LogBuffer),"%f", end.tv_sec-start.tv_sec+(end.tv_usec- start.tv_usec)/1000000.0);
                    sprintf(LogBuffer + strlen(LogBuffer)," %d \n", currentEndBuffer);
                }
            } 
            else if(numBytesRcvd == 3)
            {
                endTranmission = 1;
            }
          } while (numBytesRcvd > 0);
      }
      else
      {
        ssize_t numBytesRcvd;
          do 
          { 
            struct sockaddr_in ssin; //address
            socklen_t sendsize = sizeof(ssin); // Address length in-out parameter
            numBytesRcvd = recvfrom(clientUDPSocket, &globalBuffer[currentEndBuffer], bufferSize - currentEndBuffer+4, 0, (struct sockaddr *) &ssin, &sendsize);
            if (numBytesRcvd < 0) 
            {
              if (errno != EWOULDBLOCK)
              {
                    printf("2: recvfrom() failed");
                    // exit(1);
              }
            } 
            else if(numBytesRcvd != 3) 
            {
                endTranmission =0;
                int isGood = 0;
                int temp =  (int)((unsigned char)(globalBuffer[currentEndBuffer]) << 24 |
                                  (unsigned char)(globalBuffer[currentEndBuffer + 1]) << 16 |
                                  (unsigned char)(globalBuffer[currentEndBuffer + 2]) << 8 |
                                  (unsigned char)(globalBuffer[currentEndBuffer + 3]));
                /*checking packet*/
                if(temp < lastPacket) /*ignore*/
                {
                    isGood = 0;
                    printf("ignore %d\n", temp);
                } 
                else if(temp > lastPacket+1) /*detect missing*/
                {
                    isGood = 1;
                    lastPacket = temp; /*updating*/
                     if(retransmitFlag == 1)
                    {
                        unsigned char negACK[6];
                        negACK[0] = 'M';
                        negACK[1] = ' ';
                        negACK[2] = ((lastPacket) >> 24) & 0xFF;
                        negACK[3] = ((lastPacket) >> 16) & 0xFF;
                        negACK[4] = ((lastPacket) >> 8) & 0xFF;
                        
                        if (sendto(clientUDPSocket,negACK,6, 0,(struct sockaddr*)&ssin, sizeof(ssin)) < 0)
                        {
                            printf("Fail to send\n");
                            exit(1);
                        }
                        printf("negACK sent: %c %x%x%x%x\n",negACK[0],negACK[2],negACK[3],negACK[4],negACK[5]);
                    }
                } 
                else if (temp == lastPacket +1)
                {
                    isGood=1;
                    lastPacket = temp;
                }
                if(isGood)
                {
                    endTranmission = 0;
                    memmove(globalBuffer+currentEndBuffer, globalBuffer+currentEndBuffer+4, numBytesRcvd-4);
                    // printf("numbytes received %ld\n", numBytesRcvd);
                    sem_wait (&smp);
                    currentEndBuffer = currentEndBuffer + numBytesRcvd;
                    sem_post (&smp);
                    char confirmBack[MAX_BUF];
                    sprintf(confirmBack,"Q %d %d %f",currentEndBuffer,targetBufferSize,gammaVal);
                    if (sendto(clientUDPSocket,confirmBack,strlen(confirmBack), 0,(struct sockaddr*)&ssin, sizeof(ssin)) < 0)
                    {
                        printf("Fail to send\n");
                        exit(1);
                    }
                    gettimeofday(&end, NULL);
                    sprintf(LogBuffer + strlen(LogBuffer),"%f", end.tv_sec-start.tv_sec+(end.tv_usec- start.tv_usec)/1000000.0);
                    sprintf(LogBuffer + strlen(LogBuffer)," %d \n", currentEndBuffer);
                }
            } 
            else if(numBytesRcvd == 3)
            {
                endTranmission = 1;
            }
          } while (numBytesRcvd > 0); 
      }
    }//end else not discard
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
	if(argc != 13)
	{
		printf("Usage: <server-ip> <server-tcp-port> <client-udp-port> <payload-size> <playback-del> <gammaVal> <buf-sz> <target-buf> <logfile-s> <filename> <discard-k> <flag>\n ");
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
    payloadDelay = strtof(argv[5], NULL);
    printf("Payload delay: %f\n", payloadDelay);

    /*get gammaVal*/
    gammaVal = strtof(argv[6], NULL);
    printf("gammaVal: %f\n", gammaVal);
    
    /*buffer size*/
    bufferSize = 1000*strtol(argv[7], NULL,10);
    printf("Buffer size: %d\n", bufferSize);
    //allocate
    globalBuffer = malloc(bufferSize);
    /*target buffer size*/
    targetBufferSize = 1000*strtol(argv[8], NULL,10);
    printf("target buffer size: %d\n", targetBufferSize);
    /*log file name*/
    logFileName = argv[9];
    printf("Log File: %s\n", logFileName);
    
    /*file name*/
    fileName = argv[10];
    printf("file name: %s\n", fileName);

    /*Discard K*/
    discardK = strtol(argv[11], NULL, 10);
    printf("Discard at: %d\n", discardK);

    /*Retransmit Flag*/
    retransmitFlag = strtol(argv[12], NULL, 10);
    printf("Retransmit: %d\n", retransmitFlag);

    /***initialize semaphore*****/
    sem_init(&smp, 0, 1);
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
    /***************************Prefetching*****************************/
    int pDelay = (int) payloadDelay*1000000000;
    if(pDelay < 999999999)
    {   
        sleepTime.tv_sec = 0;
        sleepTime.tv_nsec = pDelay; //mili to nano
    }
    else
    {
        sleepTime.tv_sec = (int) pDelay/1000000000;
        sleepTime.tv_nsec = (pDelay%1000000000)*1000000; //mili to nano    
    }
    if(nanosleep(&sleepTime, &remainTime) < 0)
    {
        printf("Child: nanosleep was interupted by signal. Time left: %f \n", (remainTime.tv_sec + remainTime.tv_nsec/1000000000.0));
    }
    while((nanosleep(&remainTime,&remainTime)) != 0)
    {
        printf("Prefetching....\n");
    }
    /***************************Prefetching*****************************/

    char * audioFileName = "/dev/audio";
    audioFD = open(audioFileName, O_RDWR|O_TRUNC, 0666);
    if (audioFD < 0) 
    {
        printf("cannot open file: %s \n", audioFileName);
        exit(1);
    }
    int mu = (int) 1000/gammaVal; /*30000*/
    printf("mu values %d\n", mu);
    ualarm(mu*1000,mu*1000);
    signal(SIGALRM, SIGALARM_handler);
    /*still reading and writing*/
    while(endTranmission != 1 || currentEndBuffer != 0)
    {
    }

    /************************print to log file***********************/
    printf("End Tranmission\n");
    printf("Write to log file\n");
    int logfd;
    if ((logfd = open(logFileName,O_RDWR|O_CREAT|O_TRUNC, 0666)) < 0)
    {
        printf("Child: Cannot create file");
        exit(1);                
    }
    if(write(logfd,LogBuffer,strlen(LogBuffer)) < 0)
    {
        printf("Child: Cannot write to file");
        exit(1);
    }
    close(logfd);
    /************************print to log file***********************/
	return 0;
}
