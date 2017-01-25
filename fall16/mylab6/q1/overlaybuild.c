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
#include <time.h> 
#include <sys/time.h>

#define CLIENT_MAX_BUF 1024
#define MAX_BUF 1024

char* concatString(char *s1, char *s2)
{
    char *result = malloc(strlen(s1)+strlen(s2)+1);//+1 for the zero-terminator
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}
/* check valid ip*/
int isValidIpAddress(char *ipAddress)
{
    struct sockaddr_in sa;
    if (inet_pton(AF_INET, ipAddress, &(sa.sin_addr))<= 0)
    	return -1;
    return 0;
}


int main(int argc, char *argv[])
{
	/*socket information*/
	int s; 
	int firstOLPort;
	struct sockaddr_in sin;
	struct sockaddr_in ssin;
	/*numbyte received*/
	ssize_t numBytesRcvd;
	/*build address data structure*/
	/*checking user input*/
	if(argc < 6)
	{
		printf("Error: <dst-ip> <dst-port> <routerk-ip> ... <router2-ip> <router1-ip> <overlay-port> <build-port>");
		exit(1);
	}
	/*check last two argument*/
	if(isValidIpAddress(argv[argc-2]) == 0 || isValidIpAddress(argv[argc-1]) == 0)
	{
		printf("Error: last two arguments are supposed to be portnumbers\n");	
		exit(1);
	}
	printf("Number of overlay router: %d\n", argc-5);
	/************************building request************************************/
	int i;
	char* buildRequest;
	for(i = 1; i < argc-2; i++)
	{
		if(i == 1) // destination ip
		{
			
			if(isValidIpAddress(argv[i]) == 0)
				buildRequest = concatString("$", argv[i]);
			else
			{
				printf("error: invalid destination ip-address\n");
				exit(1);
			}
		}
		else if (i == 2) // destination port number
		{
			buildRequest = concatString(buildRequest,"$");
			buildRequest = concatString(buildRequest,argv[i]);
		}
		else if (i == argc - 3) // last overlay router
		{
			if(isValidIpAddress(argv[i]) == 0) // only concatenate ip address
			{
				buildRequest = concatString(buildRequest,"$");
				buildRequest = concatString(buildRequest,argv[i]);
				buildRequest = concatString(buildRequest,"$");
			}
			else
			{
				printf("Error: please check your input\n");
				exit(1);
			}
		}
		else
		{
			if(isValidIpAddress(argv[i]) == 0) // only concatenate ip address
			{
				buildRequest = concatString(buildRequest,"$");
				buildRequest = concatString(buildRequest,argv[i]);
			}
			else
			{
				printf("Error: there is one invalid ip in your input\n");
				exit(1);
			}	
		}
	}
	printf("Build Request: %s\n", buildRequest);
	/************************building request************************************/

	/*****************Get 1st-overlay router ip address**************************/
	printf("first overlay ip: %s\n", argv[argc-3]);
  	if(inet_pton(AF_INET, argv[argc-3], &sin.sin_addr)<=0)
    {
        printf("\n inet_pton error occured\n");
        exit(1);
    }
    /* get destination port number*/
    firstOLPort = strtol(argv[argc-2],NULL,10);
    printf("first overlay router port: %d\n", firstOLPort);  	
  	/* address family = Internet */
    sin.sin_family = AF_INET;
  	/* set port number, using htons function to use proper byte order */
  	sin.sin_port = htons(firstOLPort);
  	/* set all bits of the padding field to 0 */
  	memset(sin.sin_zero, '\0', sizeof sin.sin_zero);
	/*****************Get 1st-overlay router ip address**************************/

	
  	/*****************binding socket to build-port*******************************/
	struct sockaddr_in dcmsin;
    /* address family = Internet */
    dcmsin.sin_family = AF_INET;
  	/* set port number, using htons function to use proper byte order */
    int buildPort;
    buildPort = strtol(argv[argc-1],NULL,10);
    printf("build port: %d\n", buildPort);
  	dcmsin.sin_port = htons(buildPort);
  	/* set IP address to localhost */
  	dcmsin.sin_addr.s_addr = htons(INADDR_ANY);
  	/* set all bits of the padding field to 0 */
  	memset(dcmsin.sin_zero, '\0', sizeof dcmsin.sin_zero);
   	/* creating socket*/
   	if ((s = socket(AF_INET,SOCK_DGRAM,0)) < 0)
	{
	 	printf("Fail to create socket");
	 	exit(1);
	}
	/* binding*/
   	if ((bind(s, (struct sockaddr *)&dcmsin, sizeof(dcmsin))) < 0)
   	{
   		printf("Fail to bind");
   		exit(1);
   	}
  	/*****************binding socket to build-port*******************************/
	/* sending build request*/
	if (sendto(s,buildRequest,strlen(buildRequest),0,(struct sockaddr*)&sin, sizeof(sin)) < 0){
		printf("Fail to send\n");
		exit(1);
	}
	/* block to wait for an ACK from 1st overlay router*/
    char buf[CLIENT_MAX_BUF];
	memset(buf,0,CLIENT_MAX_BUF);
	socklen_t sendsize = sizeof(ssin);	
	if((numBytesRcvd = recvfrom(s, buf, sizeof(buf), 0, (struct sockaddr *)&ssin, &sendsize)) < 0)
	{
		printf("Fail to receive\n");
		exit(1);
	}
	/* breakdown response*/
	// printf("Response from 1st Router: %s\n", buf);
	char* token;
	char routerResponse[16];
	int16_t dstPort;
	const char d[2] = "$"; 
	/* split by delimiter*/
    token = strtok(buf, d);
    strncpy(routerResponse, token, 15);
	routerResponse[16] = '\0';
	char ipRequest[16] = "";
	while(token != NULL)
	{
		strncpy(ipRequest, token, 15);
		ipRequest[16] = '\0';
    	token = strtok(NULL, d);
	}	
	dstPort = strtol(ipRequest, NULL, 10);
	printf("First router [%s]\nUse this port: %d\n", inet_ntoa(ssin.sin_addr),dstPort);
	exit(0);
}
