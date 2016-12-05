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
/*File bytestream with Last Name Chacracter*/
unsigned char *gen_bytestream (size_t num_bytes)
{
  unsigned char *bytestream = malloc (num_bytes);
  size_t i;
  for (i = 0; i < num_bytes; i++)
  {
    bytestream[i] = 'L'; //Last Name Le
  }
  bytestream[num_bytes] ='\0';
  return bytestream;
}

int main(int argc, char *argv[])
{
	/*socket information*/
	int s; 
	int serverPort;
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
	if(argc != 7)
	{
		printf("Error: <hostname> <portnumber> <payload-size> <package-count> <package-spacing> <dedicated-port>");
		exit(1);
	}
	/*Get IP address*/
  	if(inet_pton(AF_INET, argv[1], &sin.sin_addr)<=0)
    {
        printf("\n inet_pton error occured\n");
        exit(1);
    }
    /*Get port number*/
    serverPort = strtol(argv[2],NULL,10);
    printf("Portnumber: %d\n", serverPort);
    /*Get PayLoad Size*/
    payloadSize = strtol(argv[3],NULL,10); ;
    printf("payloadSize: %d\n", payloadSize);
    /*Get Package Count*/
    packageCount = strtol(argv[4],NULL,10); ;
    printf("Package Count: %d\n", packageCount);
    /*Get Spacing*/
    packageSpacing = strtol(argv[5],NULL,10);
    printf("Package Spacing: %d\n", packageSpacing);
  	/* Address family = Internet */
    sin.sin_family = AF_INET;
  	/* Set port number, using htons function to use proper byte order */
  	sin.sin_port = htons(serverPort);
  	/* Set all bits of the padding field to 0 */
  	memset(sin.sin_zero, '\0', sizeof sin.sin_zero);
	/*Create UDP socket*/
	struct sockaddr_in thisSin;
    /*binding*/
  	/* Address family = Internet */
    thisSin.sin_family = AF_INET;
  	/* Set port number, using htons function to use proper byte order */
    int16_t thisPort;
    thisPort = strtol(argv[7],NULL,10);
  	thisSin.sin_port = htons(thisPort);
  	/* Set IP address to localhost */
  	thisSin.sin_addr.s_addr = htonl(INADDR_ANY);
  	/* Set all bits of the padding field to 0 */
  	memset(sin.sin_zero, '\0', sizeof sin.sin_zero);
   	/* creating socket*/
	if((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        printf("Failed to create socket \n");
        exit(1);
    }	
    if ((bind(s, (struct sockaddr *)&thisSin, sizeof(thisSin))) < 0)
   	{
   		perror("Fail to bind");
   		exit(1);
   	} 
    /*Payload*/ 
	unsigned char *senderRequest;
	//Fill Payload
	senderRequest = gen_bytestream(payloadSize);
	struct timeval start, end;
	/*Sending Payload*/
	//Start Sending
	int i;
	//Start
	gettimeofday(&start, NULL);
	for (i = 0; i < packageCount; i++)
	{
		if (sendto(s,senderRequest,strlen(senderRequest),0,(struct sockaddr*)&sin, sizeof(sin)) < 0){
			printf("Fail to send\n");
			exit(1);
		}
		//spacing between 2 package
		usleep(packageSpacing);
	}
	//Done Sending
	gettimeofday(&end, NULL);
	/*Signal ending of transmission by sending 3 bytes*/
	for (i = 0; i < 3; i++)
	{
		unsigned char a[3] = "123"; //payload of size 3
		if (sendto(s,a,3,0,(struct sockaddr*)&sin, sizeof(sin)) < 0){
				printf("Fail to send\n");
				exit(1);
		}
	}
	/*calculate completion time in second*/
	float completionTime = (end.tv_sec - start.tv_sec)+(end.tv_usec - start.tv_usec)/1000000.0;
	fprintf(stdout,"Completion Time: %f s\n", completionTime);
	/*PPS*/
	printf("Package Per Second (PPS): %f packages/s\n", packageCount/completionTime);
	/*Calculate total bit sent*/
	/*payloadSize, 8 bytes of UDP header, 20 bytes of IPv4 Header, 27 bytes of Ethernet Frames*/
	//Assuming DIX
	float totalBitPS = (packageCount*(payloadSize+8+20+27)*8)/completionTime;
	printf("Bit sent: %d\n", packageCount*(payloadSize+8+20+27)*8);
	/*BPS*/
	printf("Bits Per Second (BPS): %f bps\n", totalBitPS);
	exit(0);
}
