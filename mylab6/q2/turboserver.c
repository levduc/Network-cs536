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
#include <time.h>
#include <errno.h>
#include <sys/time.h>
#include "dropsendto.c"

#define MAX_BUF 100000
#define SK_MAX 20
/*global udp socket*/
const char d[2] = "$";
int32_t udpSocket;
int32_t packageCount;
int32_t leftWindowP;
/*for restranmission*/
int32_t payloadSize;
char **arr;
/*concatString is to concatenate two string together*/
char* concatString(char *s1, char *s2)
{
    char *result = malloc(strlen(s1)+strlen(s2)+1); //+1 for the zero-terminator
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}
/*SIGIOhandler*/
#define WINDOWSIZE 1000000000
void SIGIOHandler(int sig_num) 
{
  ssize_t numBytesRcvd;
  do 
  { 
    struct sockaddr_in csin; //address
    socklen_t sendsize = sizeof(csin); // Address length in-out parameter
    char buf[MAX_BUF];      // buf
    memset(buf,0,MAX_BUF);
    numBytesRcvd = recvfrom(udpSocket, buf, MAX_BUF, 0, (struct sockaddr *) &csin, &sendsize);
    if (numBytesRcvd < 0) 
    {
      if (errno != EWOULDBLOCK)
        printf("recvfrom() failed");
    } 
    else 
    {
    	/*breakdown buf*/
		if(buf[0]== 'M')
		{
            printf("Neg ACK: %c %x%x%x%x\n", buf[0], (unsigned char)buf[2], (unsigned char)buf[3], (unsigned char) buf[4], (unsigned char) buf[5]);
            int temp =  (int)  ((unsigned char)(buf[2]) << 24 |
                                (unsigned char)(buf[3]) << 16 |
                                (unsigned char)(buf[4]) << 8 |
                                (unsigned char)(buf[5]));
            if (temp > packageCount - WINDOWSIZE)
            {
            	/*temp is store at arr[temp%WINDOWSIZE]*/
	            if (sendto(udpSocket,arr[temp%WINDOWSIZE],payloadSize+4, 0,(struct sockaddr*)&csin, sizeof(csin)) < 0)
	            {
	                printf("Fail to send\n");
	                // exit(1);
	            }
	            printf("Retransmit: %x%x%x%x\n", (unsigned char)arr[temp][0], (unsigned char)arr[temp][1], (unsigned char) arr[temp][2], (unsigned char) arr[temp][3]);
            }
		}
		/*positive ack*/
		if(buf[0]=='C')
		{
			int temp =  (int)  ((unsigned char)(buf[2]) << 24 |
                                (unsigned char)(buf[3]) << 16 |
                                (unsigned char)(buf[4]) << 8 |
                                (unsigned char)(buf[5]));
			leftWindowP = temp;
		}
    }//end else
  } while (numBytesRcvd >= 0);
}

int main(int argc, char *argv[])
{
    /*child count*/
  	unsigned int childCount = 0; //child count
    /*Number of bytes received*/
    ssize_t numBytesRcvd;
    int32_t lossNum;
    /*Port Number*/
    int listenUdpSocket;
	int udpPort;
    /*Checking user input*/
    if(argc != 5)
    {
        printf("Usage: <udp-port> <secretkey> <configfile> <lossnum>\n");
        exit(1);
    } 
    
    /*****Get and Check Server Secret Key*****/
    char* s_secret_key;
    if(strlen(argv[2]) < 10){
    	printf("Secret key is too short\n");
        exit(1);
    }
    if(strlen(argv[2]) > 20){
    	printf("Secret key is too long\n");
        exit(1);
    }
    s_secret_key = argv[2];
    /*****Get and Check Server Secret Key*****/


    /*************open configfile************/
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
    payloadSize = strtol(bsize,NULL,10);
    printf("payload size: %d\n", payloadSize);
    /*************open configfile************/

    lossNum = strtol(argv[4],NULL,10);
   	/*window size*/
    arr = malloc(WINDOWSIZE*sizeof(char*));

   	/*************************udp-server***********************************/

  	/*server address*/
  	struct sockaddr_in udpSin;
    /*build address data structure*/
  	/*address family = Internet */
    udpSin.sin_family = AF_INET;
    /*server tcp-port*/
    udpPort = strtol(argv[1],NULL,10);
    printf("UDP-Port: %d\n", udpPort);
    /*set port number, using htons function to use proper byte order */
  	udpSin.sin_port = htons(udpPort);
  	/*set IP address to localhost */
  	udpSin.sin_addr.s_addr = htonl(INADDR_ANY);
  	/*set all bits of the padding field to 0 */
  	memset(udpSin.sin_zero, '\0', sizeof udpSin.sin_zero);
  	/*udp socket*/
   	if ((listenUdpSocket = socket(PF_INET,SOCK_DGRAM,0)) < 0) 
   	{
   	 	perror("Fail to create tcp socket");
   	 	exit(1);
   	} 
   	/*binding*/
   	if ((bind(listenUdpSocket, (struct sockaddr *)&udpSin, sizeof(udpSin))) < 0)
   	{
   		perror("Fail to bind");
   		exit(1);
   	}
   	/*Listening to incoming request*/
	printf("Listening ... \n");
   	/*************************udp-server***********************************/
	int status;
	int len;
	/*client Buffer*/
    char buf[MAX_BUF];

	while(1)
	{
		/*clear buffer*/
		memset(buf,0,MAX_BUF);
		/*Wait for request*/
		struct sockaddr_in csin;
    	socklen_t sendsize = sizeof(csin);
		if((numBytesRcvd = recvfrom(listenUdpSocket, buf, sizeof(buf), 0, (struct sockaddr *) &csin, &sendsize)) < 0)
		{
			printf("Fail to receive\n");
			exit(1);
		}
    	/*Receive a request*/
		printf("Receive a request from client: %s\n", buf);
		/*Fork a child to handle client request*/
	    pid_t k = fork();
		// fork failed		
		if(k < 0)
		{
			printf("fork() failed!\n");
		}
		else if (k==0) //Child Code
		{ 
	    	/*client request*/
	    	buf[numBytesRcvd] = '\0';		
		    /*Break down client request*/
		    char *token;
		    printf("%s\n", buf);
		    token = strtok(buf, d); 
			/*Getting Secret Key*/
			char c_secret_key[SK_MAX+1]; // client secret key
			strncpy(c_secret_key, token, SK_MAX+1);
			c_secret_key[SK_MAX] = '\0';
		    token = strtok(NULL, d);
		    /*Client File Name*/
			char fileName[MAX_BUF+1];
			if(token != NULL)
			{
				strncpy(fileName, token, MAX_BUF+1);
				fileName[MAX_BUF] = '\0';
			}
			else
			{
				fileName[MAX_BUF] = '\0';
			}
			/**************comparing secret key************/
			printf("Comparing Secret Key\n");
		    if(strcmp(s_secret_key, c_secret_key) != 0) 
		    {
				printf("Keys don't match\n");
		    	exit(1);
		    }
			/**************comparing secret key************/

			/*response a deny*/            
			printf("%s\n", fileName);
			char* fileDir = concatString("filedeposit/",fileName);
		    int fd;
		    if ((fd = open(fileDir,O_RDONLY, 0666)) < 0)
		    {
				printf("Child: Cannot open file. File may not exist.");
				char * deny = "KO";
				sendto(listenUdpSocket, deny, strlen(deny)+1,0,(struct sockaddr*)&csin, sizeof(csin));
				exit(1);
		    }

		    /*File exists. binding udp*/
		   	/***************************server udp**************************************/
		    struct sockaddr_in udp_sin;
		    int16_t udpPort = 20000;
    		udp_sin.sin_family = AF_INET;
	  		udp_sin.sin_addr.s_addr = htonl(INADDR_ANY);
		    udp_sin.sin_port = htons(udpPort);
	  		memset(udp_sin.sin_zero, '\0', sizeof udp_sin.sin_zero);
		   	/*create udp socket*/
		   	if ((udpSocket = socket(PF_INET,SOCK_DGRAM,0)) < 0) 
		   	{
		   	 	perror("Child: Fail to create udp socket");
		   	 	exit(1);
		   	}
		   	while ((bind(udpSocket, (struct sockaddr *) &udp_sin, sizeof(udp_sin))) < 0)
		   	{
		  		printf("Child: Port %d is busy. Rebinding using different port ...\n", udpPort);
		  		udpPort +=1;
		  		udp_sin.sin_port = htons(udpPort);
		   	}
		   	/***************************server udp**************************************/
		    			
			/***************************SIGIO handler***********************************/
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
		    if (fcntl(udpSocket, F_SETOWN, getpid()) < 0)
		      printf("Unable to set process owner to us");
		    // Arrange for nonblocking I/O and SIGIO delivery
		    if (fcntl(udpSocket, F_SETFL, O_NONBLOCK | FASYNC) < 0)
		      printf("Unable to put client sock into non-blocking/async mode");
			/***************************SIGIO handler***********************************/
			
		    
		    /*start writing*/
		  	unsigned char writeBuf[payloadSize+4];
		 	unsigned char bytes[4];
	   		memset(writeBuf, 0,payloadSize);
		 	int32_t byteRead = 0;
		 	int32_t  byteWrite = 0;
		 	int32_t firstRead = 0;

		 	while((byteRead = read(fd, &writeBuf[4], payloadSize)) > 0)
		 	{
		 		/*convert to byte*/
				bytes[0] = (packageCount >> 24) & 0xFF;
				bytes[1] = (packageCount >> 16) & 0xFF;
				bytes[2] = (packageCount >> 8) & 0xFF;
				bytes[3] = packageCount & 0xFF;
				/*add sequence number*/
		 		writeBuf[0] = bytes[0];
		 		writeBuf[1] = bytes[1];
		 		writeBuf[2] = bytes[2];
		 		writeBuf[3] = bytes[3];

		 		/*sending packet*/
				arr[packageCount%WINDOWSIZE] = malloc(sizeof(char) * strlen(writeBuf));
		 		memset(arr[packageCount%WINDOWSIZE],'\0',payloadSize+4);
				memcpy(arr[packageCount%WINDOWSIZE],writeBuf,payloadSize+4);
				printf("%x%x%x%x\n", (unsigned char)arr[packageCount][0],(unsigned char)arr[packageCount][1],(unsigned char)arr[packageCount][2],(unsigned char)arr[packageCount][3]);
		 		if(dropsendto(100,packageCount+lossNum) == 1)
		 		{
		 			packageCount++;
	   				memset(writeBuf,0, payloadSize);
	   				printf("drop");
	   				continue;
		 		}
		 	    if ((byteWrite = sendto(udpSocket,writeBuf,byteRead+4,0,(struct sockaddr*)&csin, sizeof(csin))) < 0)
		 	    {
					printf("Child: Fail to send\n");
					exit(1);
				}
				packageCount++;
			    /*keep for resend*/
	   			memset(writeBuf,0, payloadSize);
		 	}
		 	/*signal end tranmission*/
		 // 	int i;
			// for(i =0; i<3; i++)
			// {
			//  	if ((sendto(udpSocket,"EEE",3,0,(struct sockaddr*)&csin, sizeof(csin))) < 0)
		 // 	    {
			// 		printf("Child: Fail to send\n");
			// 		exit(1);
			// 	}
			// }	
			/***********************/
			// close(udpSocket);
		    close(fd);
	        printf("Child: Done sending\n");
	    	// exit(0);
    	}
	  	else if(k>0) 
	  	{	
			// parent code 
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
