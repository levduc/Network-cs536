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
// #include <limits.h>

#define MAX_BUF 100000
#define EXMAX_BUF 1000000000
#define SK_MAX 20

/*package spacing*/
float packageSpacing;
/*payload size*/
int payloadSize;
/*global udp socket*/
int udpSocket;
struct timespec sleepTime, remainTime;
const char d[2] = " ";
/*value a for method A*/
int mode;
/*Log Buffer*/
char LogBuffer[EXMAX_BUF];


/*concatString is to concatenate two string together*/
char* concatString(char *s1, char *s2)
{
    char *result = malloc(strlen(s1)+strlen(s2)+1); //+1 for the zero-terminator
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}

/*SIGIOhandler*/
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
    	/*Adjust tau here*/
    	printf("%s\n", buf);
    	/*breakdown buf*/
    	char *token;
		/*getting client-udp port*/
	    token = strtok(buf, d); 
		
		if(strcmp(token, "Q") == 0)
		{
			// printf("This is a query: %s\n", token);
		}
		else
		{
			printf("This is not a query\n");
		}
		/*get current buffer*/
	    token = strtok(NULL, d);
		char currentBufferLevel[SK_MAX];

		if(token != NULL)
		{
			strncpy(currentBufferLevel, token, SK_MAX);
			currentBufferLevel[SK_MAX] = '\0';
		}
		else
		{
			printf("No current buffer\n");
		}
	    /*get target buffer*/
	    token = strtok(NULL, d);
		char targetBuf[MAX_BUF];
		if(token != NULL)
		{
			strncpy(targetBuf, token, MAX_BUF);
			targetBuf[MAX_BUF] = '\0';
		}
		else
		{
			printf("No target buffer\n");
		}
		/*getting gamma*/
		token = strtok(NULL, d);
		char gamma[SK_MAX];
		if(token != NULL)
		{
			strncpy(gamma, token, SK_MAX);
			gamma[SK_MAX] = '\0';
		}
		else
		{
			printf("No gamma\n");
		}
		/*parse to int*/
		int currentBL = strtol(currentBufferLevel, NULL, 10);
		int targetBL = strtol(targetBuf, NULL, 10);
		float gammaVal = strtof(gamma, NULL);
		float lambda = 1000/packageSpacing;
		/*for method A*/
		float a = 1;
    	/*for method B*/
    	float b = .5;
    	/*for method C*/
    	float c = .001;
    	/*for method D*/
    	float d = .5;
    	/*Q* == Q(t) do nothing*/
    	if(currentBL == targetBL)
    	{
    		/*do nothing*/
    	}
    	/*Q(t)<Q*  increase lambda*/
    	if(currentBL < targetBL)
    	{
    		/*increase lambda*/
    		switch(mode)
    		{
    			/*method A*/
    			case 0:
		    		lambda = lambda + a;
		    		packageSpacing = 1000/lambda;		
	    			printf("decrease packet spacing:%f\n", packageSpacing);
		    		break;
    			/*method B*/
    			case 1:
	    			lambda = lambda + a;
	    			packageSpacing = 1000/lambda;
	    			printf("decrease packet spacing:%f\n", packageSpacing);
		    		break;
    			/*method C*/
		    	case 2:
	    			lambda = lambda + c*(targetBL-currentBL); 
		    		packageSpacing = 1000/lambda;
	    			printf("decrease packet spacing:%f\n", packageSpacing);
		    		break;
    			/*method D*/
		    	case 3:
	    			lambda = lambda + c*(targetBL-currentBL) -d*(lambda - gammaVal); 
		    		packageSpacing = 1000/lambda;
	    			printf("decrease packet spacing:%f\n", packageSpacing);
		    		break;
		    	default:
		    		printf("Not sure which mode\n");
    		}

    	}
    	/*Q(t)>Q*  decrease lambda*/
    	if(currentBL > targetBL)
    	{
    		/*decrease lambda*/
    		switch(mode)
    		{
    			/*method A*/
    			case 0:
		    		if ((lambda - a) > 0)
		    		{
		    			lambda = lambda - a; 
		    			packageSpacing = 1000/lambda;
		    			printf("increase packet spacing:%f\n", packageSpacing);
		    		}
		    		break;
		    	case 1:
    				/*method B*/
		    		if (lambda*b > 0)
		    		{
		    			lambda = lambda*b; 
		    			packageSpacing = 1000/lambda;
		    			printf("increase packet spacing:%f\n", packageSpacing);
		    		}
		    		break;
    			/*method C*/
		    	case 2:
		    		if(lambda + c*(targetBL-currentBL) > 0)
		    		{
		    			lambda = lambda + c*(targetBL-currentBL); 
			    		packageSpacing = 1000/lambda;
		    			printf("increase packet spacing:%f\n", packageSpacing);
		    		}
		    		break;
		    	case 3:
		    		if(lambda + c*(targetBL-currentBL) -d*(lambda - gammaVal) > 0)
		    		{
		    			lambda = lambda + c*(targetBL-currentBL) -d*(lambda - gammaVal); 
			    		packageSpacing = 1000/lambda;
		    			printf("decrease packet spacing:%f\n", packageSpacing);
		    		}
		    		break;
		    	default:
		    		printf("Not sure which mode\n");
    		}


    		/*method D*/
    	}
    }
  } while (numBytesRcvd > 0);
}

int main(int argc, char *argv[])
{
    /*child count*/
  	unsigned int childCount = 0; //child count
    /*client Buffer*/
    char buf[MAX_BUF];
    /*Number of bytes received*/
    ssize_t numBytesRcvd;
    /*Socket*/
    int tcpSocket, new_s; 
    /*Port Number*/
	int tcpPort;
	int udpPort;
	/*log file name*/
	char * logFileName;
    /*Checking user input*/
    if(argc != 7)
    {
        printf("Usage: <tcp-port> <udp-port> <payload-size> <packet-spacing> <mode> <logfile-s>\n");
        exit(1);
    } 
    /*payload Size*/
    payloadSize = strtol(argv[3],NULL,10);
    printf("Payload Size: %d\n", payloadSize);
    /*packet Spacing*/
    /*tau*/
    packageSpacing = strtof(argv[4],NULL);
    printf("Package Spacing: %f\n", packageSpacing);
    /*mode*/
    mode = strtol(argv[5],NULL,10);
    printf("Mode used %d\n", mode);
    /*logFile*/
    logFileName = argv[6];
    printf("Log File: %s\n", logFileName);
   	
   	/*************************tcp-server***********************************/
  	/*server address*/
  	struct sockaddr_in tcp_sin;
    /*build address data structure*/
  	/*address family = Internet */
    tcp_sin.sin_family = AF_INET;
    /*server tcp-port*/
    tcpPort = strtol(argv[1],NULL,10);
    printf("TCP-Port: %d\n", tcpPort);
    /*set port number, using htons function to use proper byte order */
  	tcp_sin.sin_port = htons(tcpPort);
  	/*set IP address to localhost */
  	tcp_sin.sin_addr.s_addr = htonl(INADDR_ANY);
  	/*set all bits of the padding field to 0 */
  	memset(tcp_sin.sin_zero, '\0', sizeof tcp_sin.sin_zero);
  	/*tcp socket*/
   	if ((tcpSocket = socket(PF_INET,SOCK_STREAM,0)) < 0) 
   	{
   	 	perror("Fail to create tcp socket");
   	 	exit(1);
   	} 
   	/*binding*/
   	if ((bind(tcpSocket, (struct sockaddr *)&tcp_sin, sizeof(tcp_sin))) < 0)
   	{
   		perror("Fail to bind");
   		exit(1);
   	}
   	/*Listening to incoming request*/
   	listen(tcpSocket,10);
	printf("Listening ... \n");
   	/*************************tcp-server***********************************/
	int status;
	int len;
	/*delimiter*/
    /*server udp-port*/
    udpPort = strtol(argv[2],NULL,10);
    printf("initial UDP-Port: %d\n", udpPort);
	
	while(1)
	{
		/*clear buffer*/
		memset(buf,0,MAX_BUF);
		/*creating new socket*/
		if((new_s = accept(tcpSocket, (struct sockaddr *) &tcp_sin, &len)) < 0)
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
		printf("Receive a request from client: %s\n", buf);
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
	    	/*client request*/
	    	buf[numBytesRcvd] = '\0';		
		    /*break down client request*/
		    char *token;
		    token = strtok(buf, d); 
			/*getting client-udp port*/
			char clientUDPPort[SK_MAX+1];
			strncpy(clientUDPPort, token, SK_MAX+1);
			clientUDPPort[SK_MAX] = '\0';
		    token = strtok(NULL, d);
		    /*client File Name*/
			char fileName[MAX_BUF+1];
			if(token != NULL)
			{
				strncpy(fileName, token, MAX_BUF+1);
				fileName[MAX_BUF] = '\0';
			}
			else
			{
				printf("Child: resspone KO\n");
				exit(1);
			}
            
		    int fd;
		    if ((fd = open(fileName,O_RDONLY, 0666)) < 0)
		    {
				printf("Child: Cannot open file. File may not exist.");
				char * deny = "KO";
				write(new_s, deny, strlen(deny));
				/*close*/
				close(new_s);
				exit(1);
		    }
		    /*File exists. binding udp*/
		   	/***************************server udp**************************************/
		    struct sockaddr_in udp_sin;
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
		    
		    /***************************client udp**************************************/
		    struct sockaddr_in udp_csin;
    		udp_csin.sin_family = AF_INET;
	  		udp_csin.sin_addr.s_addr = tcp_sin.sin_addr.s_addr;
	  		int clientUdpPortInt = strtol(clientUDPPort,NULL,10);
		    udp_csin.sin_port = htons(clientUdpPortInt);
		    printf("%d\n", clientUdpPortInt);
		    /***************************client udp**************************************/
			
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
			
			/*Sending confirmation*/
			printf("Child: File exists. Sending confirmation ....\n");
		    char confirmation[SK_MAX];
	    	sprintf(confirmation,"OK %d", udpPort);
	    	printf("%s\n", confirmation);
			write(new_s, confirmation, strlen(confirmation));
			close(new_s);
			/********************************************************/

		    /*start writing*/
		  	unsigned char writeBuf[payloadSize];
	   		memset(writeBuf, 0, payloadSize);
		 	int byteRead = 0;
		 	int  byteWrite = 0;
		 	/*to write to log file*/
			struct timeval start, end;
		 	int firstRead = 0;
		 	memset(LogBuffer,0,EXMAX_BUF);
		 	while((byteRead = read(fd, writeBuf, payloadSize)) > 0)
		 	{
		 		if(firstRead == 0)
		 		{
		 	    	gettimeofday(&start, NULL);
		 	    	firstRead = 1;
		 		}
		 	    if ((byteWrite = sendto(udpSocket,writeBuf,byteRead,0,(struct sockaddr*)&udp_csin, sizeof(udp_csin))) < 0)
		 	    {
					printf("Child: Fail to send\n");
					exit(1);
				}
		 	   	gettimeofday(&end, NULL);

				sprintf(LogBuffer + strlen(LogBuffer),"%f", end.tv_sec-start.tv_sec+(end.tv_usec- start.tv_usec)/1000000.0);
				sprintf(LogBuffer + strlen(LogBuffer)," %f \n", 1000/packageSpacing);
				/******************************nanosleep*******************************/
				int tau = (int) packageSpacing*1000000;
				if(tau < 999999999)
				{	
					sleepTime.tv_sec = 0;
					sleepTime.tv_nsec = tau; //mili to nano
				}
				else
				{
					sleepTime.tv_sec = tau/1000000000;
					sleepTime.tv_nsec = (tau%1000000000)*1000000; //mili to nano	
				}
				if(nanosleep(&sleepTime,&remainTime) < 0)
				{
					printf("Child: nanosleep was interupted by signal. Time left: %f \n", (remainTime.tv_sec + remainTime.tv_nsec/1000000000.0));
				}
				while((nanosleep(&remainTime,&remainTime)) != 0)
			    {
			        printf("I need to finish sleeping\n");
			    }
				/******************************nanosleep*******************************/
				printf("Child Num byte sent %d, packet-spacing %f\n", byteWrite, packageSpacing);
	   			memset(writeBuf,0, payloadSize);
		 	}
		 	/*signal end tranmission*/
			int i;
			for(i =0; i<3; i++)
			{
			 	if ((sendto(udpSocket,"EEE",3,0,(struct sockaddr*)&udp_csin, sizeof(udp_csin))) < 0)
		 	    {
					printf("Child: Fail to send\n");
					exit(1);
				}
			}	
			/***********************/
			//close connection
			close(new_s);
			close(udpSocket);
		    close(fd);
	        memset(fileName,0,MAX_BUF);
	        printf("Child: Done sending\n");
	        printf("Child: write to log file\n");
	        
	        int logfd;
		    if ((logfd = open(logFileName,O_RDWR | O_CREAT|O_TRUNC, 0666)) < 0)
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
    return 0;
}
