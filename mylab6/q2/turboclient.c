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
        printf("dcm sao ko viet\n");
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
  printf("ahihi\n");
  packageCount++;
    // if(packageCount%discardK == 0) /*ignore*/
    // {
    //     struct sockaddr_in ssin; //address
    //     socklen_t sendsize = sizeof(ssin); // Address length in-out parameter
    //     char tempBuf[MAX_BUF];
    //     int dcm = recvfrom(clientUDPSocket, tempBuf, payloadSize+4, 0, (struct sockaddr *) &ssin, &sendsize);
    //     if(dcm != 3)
    //     {
    //           printf("Discard %x%x%x%x\n", (unsigned char)tempBuf[0],(unsigned char)tempBuf[1],(unsigned char)tempBuf[2],(unsigned char)tempBuf[3]);  
    //     }
    // }
    // else /*do work*/
    // {     
      if(currentEndBuffer + payloadSize < bufferSize)
      {
          ssize_t numBytesRcvd;
          do 
          { 
            struct sockaddr_in ssin; //address
            socklen_t sendsize = sizeof(ssin); // Address length in-out parameter
            char * temp = malloc(payloadSize+4);
            // memset(temp,0,payloadSize+4);
            numBytesRcvd = recvfrom(clientUDPSocket, &globalBuffer[currentEndBuffer], payloadSize+4, 0, (struct sockaddr *)&ssin, &sendsize);
            if (numBytesRcvd < 0) 
            {
              if (errno != EWOULDBLOCK)
              {
                    printf("recvfrom() failed");
                    // exit(1);
              }
            } 
            else if(numBytesRcvd != 3) 
            {
                endTranmission =0;
                int isGood = 0;
                int32_t temp =  (int)((unsigned char)(globalBuffer[currentEndBuffer]) << 24 |
                                  (unsigned char)(globalBuffer[currentEndBuffer + 1]) << 16 |
                                  (unsigned char)(globalBuffer[currentEndBuffer + 2]) << 8 |
                                  (unsigned char)(globalBuffer[currentEndBuffer + 3]));
                printf("%d\n", temp);
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
                        negACK[5] = (lastPacket) & 0xFF;
                        
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
                }
            } 
            else if(numBytesRcvd == 3)
            {
                endTranmission = 1;
            }
          } while (numBytesRcvd > 0); 
      }
    // }//end else not discard
 }

int main(int argc, char *argv[])
{
    /*Client Buffer*/ 
    char clientBuf[MAX_BUF];
    // struct hostent *hp;
    struct sockaddr_in udpServerSin, udp_csin, udp_ssin;
    /*server port*/
    int udpServerPort, clientUdpPort;
    /*Build address data structure*/
    /*Check for client input*/
	if(argc != 6)
	{
		printf("Usage: <hostname> <portnumber> <secretkey> <filename> <configfile>\n ");
		exit(1);
	}
	/*******************get server-ip address*****************/
  	if(inet_pton(AF_INET, argv[1], &udpServerSin.sin_addr)<=0)
    {
        printf("inet_pton error occured\n");
        exit(1);
    }
    /*******************get server-ip address*****************/

    /******************get server-udp-port number************/
    udpServerPort = strtol(argv[2],NULL,10); 
   	printf("Server udp portnum: %d\n", udpServerPort);
    /******************get server-udp-port number************/

    /***********check and get client secret key**************/
    char* c_secret_key; 
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
    printf("Secret Key: %s\n", c_secret_key);
    /***********check and get client secret key**************/
    
    /**************************get file name*******************/
    char* fileName;
    //16 character
    if(strlen(argv[4]) > 16)
    {
        printf("FileName cannot have more than 16 characters\n");
        exit(1);
    }
    //check spacing and forward slash
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
    printf("Filename: %s\n", fileName);
    /**************************get file name*******************/

    /*************open configfile************/
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
    payloadSize = strtol(bsize,NULL,10);
    printf("payload size: %d\n", payloadSize);
    /*************open configfile************/
    
    /*get gammaVal*/
    // gammaVal = strtof(argv[6], NULL);
    // printf("gammaVal: %f\n", gammaVal);
    /**********Sliding window on client*********************/
    /**********Sliding window on client*********************/

    /***************allocate global buffer*****************/
    /*buffer size*/
    bufferSize = 400000;
    printf("Buffer size: %d\n", bufferSize);
    //allocate
    globalBuffer = malloc(bufferSize);
    /*target buffer size*/
    targetBufferSize = 350000;
    printf("target buffer size: %d\n", targetBufferSize);
    /***************allocate global buffer*****************/    

    /***initialize semaphore*****/
    sem_init(&smp, 0, 1);
    /***********************tcp server*********************************/
    /* Address family = Internet */
    udpServerSin.sin_family = AF_INET;
    /* Set port number, using htons function to use proper byte order */
  	udpServerSin.sin_port = htons(udpServerPort);
    /* Set all bits of the padding field to 0 */
  	memset(udpServerSin.sin_zero, '\0', sizeof udpServerSin.sin_zero);
    /***********************tcp server*********************************/
    /*ucp socket*/
    // int udpSocket;
    // ssize_t total;

    /***********************udp client*********************************/
    /* Address family = Internet */
    // udp_csin.sin_family = AF_INET;
    // /* Set port number, using htons function to use proper byte order */
    // udp_csin.sin_port = htons(clientUdpPort);
    // /* Set IP address to localhost */
    // udp_csin.sin_addr.s_addr = htonl(INADDR_ANY);
    // /* Set all bits of the padding field to 0 */
    // memset(udp_csin.sin_zero, '\0', sizeof udp_csin.sin_zero);
    // if ((clientUDPSocket = socket(AF_INET,SOCK_DGRAM,0)) < 0)
    // {
    //     printf("Fail to create socket");
    //     exit(1);
    // }
    // //Binding 
    // if ((bind(clientUDPSocket, (struct sockaddr *)&udp_csin, sizeof(udp_csin))) < 0)
    // {
    //     printf("Fail to bind");
    //     exit(1);
    // }
    /***********************udp client*********************************/

    /*Client Request*/
    char clientRequest[MAX_BUF];
    int len;
    const char d[2] = " ";

    if((clientUDPSocket = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        printf("Failed to create socket \n");
        exit(1);
    }

    /*******************build and send client request***********************/
    sprintf(clientRequest,"$%s$", c_secret_key);
    //Concatenate with filename
    char * fileRequest = concatString(clientRequest,fileName);
    if (sendto(clientUDPSocket,fileRequest,strlen(fileRequest)+1
        ,0,(struct sockaddr*)&udpServerSin, sizeof(udpServerSin)) < 0)
    {
        printf("Fail to send\n");
        exit(1);
    }
    //free request
    free(fileRequest);
    /*******************build and send client request***********************/

    int32_t numBytesRcvd;
    //file descriptor
    int fd;
    char serverBuf[MAX_BUF];
    memset(serverBuf, '\0', MAX_BUF);

    struct sockaddr_in tempServerSin; //address
            socklen_t sendsize = sizeof(tempServerSin); // Address length in-out parameter
    
    // if((numBytesRcvd = recvfrom(udpSocket, serverBuf, MAX_BUF, 0,(struct sockaddr *) &tempServerSin, &sendsize)) < 0)
    // {
    //     printf("Error receiving.");
    //     exit(1);
    // }
    // printf("%s\n", serverBuf);
    /*server buffer*/
    // serverBuf[numBytesRcvd] = '\0';       

    /*break down server buffer*/
    // char *token;
    // token = strtok(serverBuf, d); 
    /*getting*/
    // char serverAnswer[2];
    // strncpy(serverAnswer, token, 2);

    // serverAnswer[2] = '\0';
    // token = strtok(NULL, d);
    /*client File Name*/
    // char serverPortNum[SK_MAX];
    // if(token != NULL)
    // {
    //     strncpy(serverPortNum, token, SK_MAX);
    //     serverPortNum[SK_MAX] = '\0';
    // }
    // else
    // {
    //     printf("File doesn't exist\n");
    //     exit(1);
    // }

    // /***********************udp server*********************************/
    // /* Address family = Internet */
    // udp_ssin.sin_family = AF_INET;
    // /*get server-ip address*/
    // if(inet_pton(AF_INET, argv[1], &udp_ssin.sin_addr) <=0)
    // {
    //     printf("inet_pton error occured\n");
    //     exit(1);
    // }
    //  Set port number, using htons function to use proper byte order 
    // udp_ssin.sin_port = htons(strtol(serverPortNum,NULL,10));
    // /* Set IP address to localhost */
    // udp_ssin.sin_addr.s_addr = htonl(INADDR_ANY);
    // /* Set all bits of the padding field to 0 */
    // memset(udp_ssin.sin_zero, '\0', sizeof udp_ssin.sin_zero);
    // /***********************udp server*********************************/


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
    
    // /***************************Prefetching*****************************/
    // int pDelay = (int) payloadDelay*1000000000;
    // if(pDelay < 999999999)
    // {   
    //     sleepTime.tv_sec = 0;
    //     sleepTime.tv_nsec = pDelay; //mili to nano
    // }
    // else
    // {
    //     sleepTime.tv_sec = (int) pDelay/1000000000;
    //     sleepTime.tv_nsec = (pDelay%1000000000)*1000000; //mili to nano    
    // }
    // if(nanosleep(&sleepTime, &remainTime) < 0)
    // {
    //     printf("Child: nanosleep was interupted by signal. Time left: %f \n", (remainTime.tv_sec + remainTime.tv_nsec/1000000000.0));
    // }
    // while((nanosleep(&remainTime,&remainTime)) != 0)
    // {
    //     printf("Prefetching....\n");
    // }
    // /***************************Prefetching*****************************/

    // char * audioFileName = "/dev/audio";
    audioFD = open("dcm", O_CREAT|O_RDWR|O_TRUNC, 0666);
    if (audioFD < 0) 
    {
        printf("cannot open file: %s \n", fileName);
        exit(1);
    }

    int32_t mu = 30000; /*30000*/
    printf("mu values %d\n", mu);
    ualarm(mu*1000,mu*1000);
    signal(SIGALRM, SIGALARM_handler);
    /*still reading and writing*/
    while(endTranmission != 1 || currentEndBuffer != 0)
    {
    }

    // /************************print to log file***********************/
    // printf("End Tranmission\n");
    // printf("Write to log file\n");
    // int logfd;
    // if ((logfd = open(logFileName,O_RDWR|O_CREAT|O_TRUNC, 0666)) < 0)
    // {
    //     printf("Child: Cannot create file");
    //     exit(1);                
    // }
    // if(write(logfd,LogBuffer,strlen(LogBuffer)) < 0)
    // {
    //     printf("Child: Cannot write to file");
    //     exit(1);
    // }
    // close(logfd);
    /************************print to log file***********************/
	return 0;
}
