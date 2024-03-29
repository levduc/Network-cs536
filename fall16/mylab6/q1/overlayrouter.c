#include <time.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <netdb.h>

#define CLIENT_MAX_BUF 2048
#define MAX_BUF 4096
int isComplete = 0;
/*concate two string function*/
char* concatString(char *s1, char *s2)
{
    char *result = malloc(strlen(s1)+strlen(s2)+1);//+1 for the zero-terminator
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}


void alarmHandler(int sig_num)
{
    if(sig_num == SIGALRM)
    {
    	if(isComplete == 0)
    	{
	    	printf("Error: Time out. Row [%d]: removed \n", getpid());
	    	exit(1);
    	}
    }
    
}

int isValidIpAddress(char *ipAddress)
{
    struct sockaddr_in sa;
    if (inet_pton(AF_INET, ipAddress, &(sa.sin_addr))<= 0)
    	return -1;
    return 0;
}

int main(int argc, char *argv[])
{
	int status;
	/*Address*/
  	struct sockaddr_in sin;
  	struct sockaddr_in csin;
  	/*socket or fd*/
    int s;
	int routerPort;
	/*Number of byte received*/
    ssize_t numBytesRcvd;
	/*delimtier*/
	const char d[2] = "$"; 
	/*buffer*/
    char buf[CLIENT_MAX_BUF];
    /*Child Count*/
  	unsigned int childCount = 0;
	char hostname[100];
	gethostname(hostname, 100* sizeof(hostname));
	struct hostent *he = gethostbyname(hostname);
	struct in_addr **addr_list;
	printf("Official name is: %s\n", he->h_name);
    printf("    IP addresses: ");
    addr_list = (struct in_addr **)he->h_addr_list;
    int i;
    for(i = 0; addr_list[i] != NULL; i++) {
        printf("%s ", inet_ntoa(*addr_list[i]));
    }
    printf("\n");
    /* check user input*/
    if(argc != 2)
    {
        printf("\n Usage: <server-port>\n");
        exit(1);
    } 
    /* string to int*/
    routerPort = strtol(argv[1],NULL,10);    
    printf("Port Number: %d\n", routerPort);
    /* build address data structure*/
  	/* address family = Internet */
    sin.sin_family = AF_INET;
  	/* set port number, using htons function to use proper byte order */
  	sin.sin_port = htons(routerPort);
  	/* set IP address to localhost */
  	sin.sin_addr.s_addr = htons(INADDR_ANY);

  	/* set all bits of the padding field to 0 */
  	memset(sin.sin_zero, '\0', sizeof sin.sin_zero);
   	
   	/* creating socket*/
   	if ((s = socket(AF_INET,SOCK_DGRAM,0)) < 0)
	{
	 	printf("Fail to create socket");
	 	exit(1);
	}
	/* binding*/ 
   	if ((bind(s, (struct sockaddr *)&sin, sizeof(sin))) < 0)
   	{
   		printf("Fail to bind");
   		exit(1);
   	}

	printf("listening ... \n");
   

	while(1)
	{
		/* clear buffer*/
		memset(buf,0,CLIENT_MAX_BUF);
		/* clock call*/
		socklen_t sendsize = sizeof(csin);
		if((numBytesRcvd = recvfrom(s, buf, sizeof(buf), 0, (struct sockaddr *) &csin, &sendsize)) < 0)
		{
			exit(1);
		}
		printf("Receive: %s\n", buf);
		/* fork a new child process to handle request*/
		pid_t k = fork(); 
		/* fail to create a child process*/
		if(k < 0) 
		{
		 	printf("fork() failed!\n");
		}
		else if (k == 0)  
		{ 
			
			/* child code*/
			/* breakdown build-request request*/
			char* token;
			char buildRequest[16];
			char* forwardBuildRequest="";
			int16_t dstPort;
			/* split by delimiter*/
		    token = strtok(buf, d);
		    strncpy(buildRequest, token, 15);
			buildRequest[16] = '\0';
			char ipRequest[16] = "";
			int count = 0;
			while(token != NULL)
			{
				strncpy(ipRequest, token, 15);
				ipRequest[16] = '\0';
				if(isValidIpAddress(ipRequest) == 0)
				{
				    count++;
				}
				if(isValidIpAddress(ipRequest) != 0)
				{
					dstPort = strtol(ipRequest, NULL, 10);
				}
		    	token = strtok(NULL, d);
		    	if(token != NULL)
		    	{
					forwardBuildRequest = concatString(forwardBuildRequest, "$");
					forwardBuildRequest = concatString(forwardBuildRequest, ipRequest);
		    	}
			}
			forwardBuildRequest = concatString(forwardBuildRequest, "$");
			/************************getting forward ip *****************************/
			char* copyForwardRequest;
			char* dcm;
			char ipForward[16] = "";
			copyForwardRequest = malloc(strlen(forwardBuildRequest)+1);
			strcpy(copyForwardRequest,forwardBuildRequest);
		    dcm = strtok(copyForwardRequest, d);
			while(dcm != NULL)
			{
				if(isValidIpAddress(dcm) == 0)
				{
					strncpy(ipForward, dcm, 15);
					ipForward[16] = '\0';
				}
		    	dcm = strtok(NULL, d);
			}
			/************************getting forward ip *****************************/

			/****************************checking ip address*************************/
			struct ifaddrs *ifap, *ifa;
			struct sockaddr_in *sa;
			char *addr;
			/*same ip*/
			int isSame = -1;
			getifaddrs (&ifap);
			for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
			   if (ifa->ifa_addr->sa_family==AF_INET) {
			       sa = (struct sockaddr_in *) ifa->ifa_addr;
			       addr = inet_ntoa(sa->sin_addr);
			       /*must match at some point*/
			       if(strcmp(addr, ipRequest) == 0)
			       {
			    		isSame = 1;
			    		break;
			       }
			   }
			}
			freeifaddrs(ifap);
			if(isSame < 0)
			{
				printf("ip addresses do not match. request is discarded\n");
				exit(1);
			}
			/****************************checking ip address*************************/
			/* checking forward ip*/
			if(isValidIpAddress(ipForward) != 0)
			{
				printf("ip is not an valid ip address %s\n",ipForward);
				exit(1);
			}
			/* ip addresses are matched*/			
			printf("fwd-router: %s\n", ipForward);
			printf("forward request: %s\n", forwardBuildRequest);
			printf("src-ip src-port: %s %d\n", inet_ntoa(csin.sin_addr), ntohs(csin.sin_port));
			// printf("number of ip in request %d\n", count);
			
			/*************************build forwarding address****************************/
  			struct sockaddr_in forwardSin;
		    /* build address data structure*/
		  	/* address family = Internet */
		    forwardSin.sin_family = AF_INET;
  			/* ip*/
			if(inet_pton(AF_INET, ipForward, &forwardSin.sin_addr)<=0)
		    {
		        printf("\n inet_pton error occured\n");
		        exit(1);
		    }
		    /* port*/
  			if(count > 2)
  				/*well-known port*/
		  		forwardSin.sin_port = htons(routerPort);
		  	else
		  	{
		  		printf("this is last overlay. dstPort %d\n", dstPort);
		  		forwardSin.sin_port = htons(dstPort);
		  	}
		  	/* set all bits of the padding field to 0 */
		  	memset(forwardSin.sin_zero, '\0', sizeof forwardSin.sin_zero);
			/*************************build forwarding address****************************/

		   	/*************************build dedicated address*****************************/
  			struct sockaddr_in dedicatedSin;
  			int overlaySock;
			/*Get random port number between 10001 and 25000*/
			srand((unsigned) time(NULL));
			int portNumber = rand()%15000+10001;
		    /* build address data structure*/
		  	/* Address family = Internet */
		    dedicatedSin.sin_family = AF_INET;
		  	/* Set port number, using htons function to use proper byte order */
		  	dedicatedSin.sin_port = htons(portNumber);
		  	/* Set IP address to localhost */
		  	dedicatedSin.sin_addr.s_addr = htonl(INADDR_ANY);
		  	/* Set all bits of the padding field to 0 */
		  	memset(sin.sin_zero, '\0', sizeof sin.sin_zero);
		   	/* creating socket*/
		   	if ((overlaySock = socket(AF_INET,SOCK_DGRAM,0)) < 0)
			{
			 	printf("Fail to create socket");
			 	exit(1);
			}
			/*need to bind this socket*/
		   	while(bind(overlaySock, (struct sockaddr *) &dedicatedSin, sizeof(dedicatedSin)) < 0)
		   	{
		   		printf("Fail to bind this port. Changing port number \n");
		   		portNumber +=1;
		  		dedicatedSin.sin_port = htons(portNumber);	
		   	}
		   	/************************build dedicated address*******************************/
		   	
		   	/************************forward request***************************************/
		   	if(count > 2) /*not last overlay*/
		   	{
			   	if (sendto(overlaySock
			   				,forwardBuildRequest
			   		       	,strlen(forwardBuildRequest),0
			   		       	,(struct sockaddr*)&forwardSin, sizeof(forwardSin)) < 0)
			   	{
					printf("Fail to send\n");
					exit(1);
				}
		   	}
		   	/************************forward request***************************************/

		   	/************************last ol-sending confirm back************************/
		   	if(count == 2)/*last overlay*/
		   	{
		   		/*send back confirmation*/
		   		alarm(30);
				signal(SIGALRM, alarmHandler);
			   	char pathConfirm[40];
				printf("Router [%s]: row [%d]|(%s:%d)<--->(%s:%d)|pending\n",ipRequest,getpid(),inet_ntoa(csin.sin_addr), ntohs(csin.sin_port), ipForward, portNumber);
			   	sprintf(pathConfirm,"$$%s$%d$",ipRequest, ntohs(dedicatedSin.sin_port));
				if (sendto(overlaySock
							,pathConfirm,strlen(pathConfirm)+1
							,0,(struct sockaddr*)&csin, sizeof(csin)) < 0){
					printf("Child: fail to send\n");
					exit(1);
				}
			   	printf("[Last Router]: Confirmation %s is sent to [%s:%d]\n", pathConfirm, inet_ntoa(csin.sin_addr),ntohs(csin.sin_port));
		   	}
		   	/************************last ol-sending confirm back************************/
		   	
			/**************************waiting for path confirmation*********************/
			if(count > 2)
			{
				int32_t bytesRcvd;
				printf("Router [%s]: row [%d]|(%s:%d)<--->(%s:%d)|pending\n",ipRequest,getpid(),inet_ntoa(csin.sin_addr), ntohs(csin.sin_port), ipForward, portNumber);
				char confirmBuff[MAX_BUF];
				memset(confirmBuff,0,MAX_BUF);
				/*set alarm here*/
				/*no confirmation then abandon path*/
    			alarm(30);
				signal(SIGALRM, alarmHandler);
	  			struct sockaddr_in snd_in;
				socklen_t send_size = sizeof(snd_in);
				if((bytesRcvd = recvfrom(overlaySock, confirmBuff, sizeof(confirmBuff),0, (struct sockaddr *) &snd_in, &send_size)) > 0)
				{
					/*comparing address*/
					if(strcmp(inet_ntoa(forwardSin.sin_addr),inet_ntoa(snd_in.sin_addr)) == 0) // if match
					{
						printf("Confirmation received: %s from address [%s:%d].\n"
								,confirmBuff, inet_ntoa(forwardSin.sin_addr), ntohs(forwardSin.sin_port)); 
						/*updating port*/
						char* token1;
						char routerResponse[16];
						/* split by delimiter*/
					    token1 = strtok(confirmBuff, d);
					    strncpy(routerResponse, token1, 15);
						routerResponse[16] = '\0';
						int16_t newPort;
						char dcm[16] = "";
						while(token1 != NULL)
						{
							strncpy(dcm, token1, 15);
							dcm[16] = '\0';
					    	token1 = strtok(NULL, d);
						}	
						newPort = strtol(dcm, NULL, 10);
						forwardSin.sin_port = htons(newPort);

						/*sending path comfirmation to previous router*/
						char pathConfirm[40];
					   	sprintf(pathConfirm,"$$%s$%d$",ipRequest,ntohs(dedicatedSin.sin_port));
						if (sendto(overlaySock
									,pathConfirm,strlen(pathConfirm)
									,0,(struct sockaddr*)&csin, sizeof(csin)) < 0){
							printf("Child: fail to send\n");
							exit(1);
						}
						printf("Router [%s]: row [%d]|(%s:%d)<--->(%s:%d)|confirmed\n", ipRequest, getpid(),inet_ntoa(csin.sin_addr), ntohs(csin.sin_port), ipForward, portNumber);
			   			printf("Confirmation sent: %s to address [%s:%d]\n", pathConfirm, inet_ntoa(csin.sin_addr),ntohs(csin.sin_port));
					}
					else /*there is a miss match*/
					{
						printf("ip addresses are missmatched\n");
						exit(1);
					}
				}
			}
			/**************************waiting for path confirmation*********************/

			/**************************waiting for data to go through********************/
			char snd_buf[MAX_BUF];
			int32_t bytesRcvd;
			struct sockaddr_in ssend_sin;
			socklen_t send_size = sizeof(ssend_sin);
			memset(snd_buf,0,MAX_BUF);
			
			char fromIP[INET_ADDRSTRLEN];
			memset(fromIP,'\0',INET_ADDRSTRLEN);
			inet_ntop(AF_INET, &(csin.sin_addr), fromIP, INET_ADDRSTRLEN);
			/************************/
			struct timeval start, end;
			gettimeofday(&start, NULL);
			while((bytesRcvd = recvfrom(overlaySock, snd_buf, sizeof(snd_buf), 0, (struct sockaddr *)&ssend_sin, &send_size)) > 0)
			{
				char tempIP[INET_ADDRSTRLEN];
				memset(tempIP,'\0',INET_ADDRSTRLEN);
				inet_ntop(AF_INET, &(ssend_sin.sin_addr), tempIP, INET_ADDRSTRLEN);

				/* traffic start to flow*/
				/* packet from forward router*/
				if(((strcmp(tempIP,ipForward) == 0) && (ntohs(ssend_sin.sin_port) == ntohs(forwardSin.sin_port)))) 
				{
					if(isComplete == 0)
					{
						if(count == 2)
							printf("Router [%s]: row [%d]|(%s:%d)<--->(%s:%d)|confirmed\n"
								,ipRequest,getpid(),inet_ntoa(csin.sin_addr), ntohs(csin.sin_port), ipForward, portNumber);
						isComplete = 1;
						printf("Path is set up. Router IP: %s\n", ipRequest);

					}
					if (sendto(overlaySock,snd_buf,strlen(snd_buf),0,(struct sockaddr*) &csin, sizeof(csin)) < 0){
						printf("Fail to send\n");
						exit(1);
					}
					printf("Router [%s]: [%s:%d] ---> [%s:%d].", ipRequest, tempIP,ntohs(ssend_sin.sin_port)
							,inet_ntoa(csin.sin_addr),ntohs(csin.sin_port));	
					gettimeofday(&end, NULL);
			    	fprintf(stdout,"Timestamp: %f ms\n", (end.tv_sec - start.tv_sec)*1000 + 
			              ((end.tv_usec - start.tv_usec)/1000.0));
				}
				/* packet from previous router*/
				if(((strcmp(tempIP, fromIP) == 0) && (ntohs(ssend_sin.sin_port) == ntohs(csin.sin_port)))) 
				{
					if(isComplete == 0)
					{
						if(count == 2)
							if(count == 2)
							printf("Router [%s]: row [%d]|(%s:%d)<--->(%s:%d)|confirmed\n"
								,ipRequest,getpid(),inet_ntoa(csin.sin_addr), ntohs(csin.sin_port), ipForward, portNumber);
								
						isComplete = 1;
						printf("Path is set up. Router IP: %s\n", ipRequest);
					}
					if (sendto(overlaySock,snd_buf,strlen(snd_buf),0,(struct sockaddr*)&forwardSin, sizeof(forwardSin)) < 0)
					{
						printf("Fail to send\n");
						exit(1);
					}
					printf("Router [%s]: [%s:%d] ---> [%s:%d].", ipRequest, 
							fromIP,ntohs(csin.sin_port),ipForward,ntohs(forwardSin.sin_port));
					gettimeofday(&end, NULL);
			    	fprintf(stdout,"Timestamp: %f ms\n", (end.tv_sec - start.tv_sec)*1000 + 
			              ((end.tv_usec - start.tv_usec)/1000.0));
				}
				memset(snd_buf,'\0',MAX_BUF);
			}
			/**************************waiting for data to go through********************/

			/* done sending kill child process*/
			// printf("Child is done! \n");
			exit(0);	
    	}
	  	else if(k>0) 
	  	{	
			/* parent code*/ 
		    /* terminate child process*/
    		childCount++;
	     	/* clean up zombies*/
	        while(childCount)
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
