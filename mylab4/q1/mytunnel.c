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

int main(int argc, char *argv[])
{
	/*socket information*/
	int s; 
	int serverPort;
	int vpn_serverPort;
	struct sockaddr_in sin;
	struct sockaddr_in ssin;
	/*Payload Size*/
	uint32_t payloadSize;
	/*Count*/
	uint32_t packageCount;
	/*Spacing*/
	uint32_t packageSpacing;
	/*numbyte received*/
	ssize_t numBytesRcvd;
	/*build address data structure*/
	/*checking user input*/
	if(argc != 5)
	{
		printf("Error: <vpn-IP> <vpn-port> <server-IP> <serverPort>");
		exit(1);
	}
	/*Get IP address*/
  	if(inet_pton(AF_INET, argv[1], &sin.sin_addr)<=0)
    {
        printf("\n inet_pton error occured\n");
        exit(1);
    }
    /*Get port number*/
    vpn_serverPort = strtol(argv[2],NULL,10);
    printf("Portnumber: %d\n", vpn_serverPort);
  	/*Create Request $ip$serverport*/
	char portRequest[10];
	//server_ip
	char *server_ip = argv[3];
	printf("%s\n", server_ip);

	sprintf(portRequest,"$%s$", argv[4]);
	//server_port
	char * mytunnelRequest;
	mytunnelRequest = concatString(portRequest, server_ip);
	printf("%s\n", mytunnelRequest);
	
  	/* Address family = Internet */
    sin.sin_family = AF_INET;
  	/* Set port number, using htons function to use proper byte order */
  	sin.sin_port = htons(vpn_serverPort);
  	/* Set all bits of the padding field to 0 */
  	memset(sin.sin_zero, '\0', sizeof sin.sin_zero);
	/*Create UDP socket*/
	if((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        printf("Failed to create socket \n");
        exit(1);
    }
	/*Sending Request*/
	//Start Sending
	if (sendto(s,mytunnelRequest,strlen(mytunnelRequest),0,(struct sockaddr*)&sin, sizeof(sin)) < 0){
		printf("Fail to send\n");
		exit(1);
	}
	
	/*Block to wait for an ACK*/
    char buf[CLIENT_MAX_BUF];
	memset(buf,0,CLIENT_MAX_BUF);
	socklen_t sendsize = sizeof(ssin);	
	if((numBytesRcvd = recvfrom(s, buf, sizeof(buf), 0, (struct sockaddr *)&ssin, &sendsize)) < 0)
	{
		printf("Fail to receive\n");
		exit(1);
	}
	printf("%s\n", buf);

	exit(0);
}
