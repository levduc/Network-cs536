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

/*concatString is to concatenate two string together*/
char* concatString(char *s1, char *s2)
{
    char *result = malloc(strlen(s1)+strlen(s2)+1);//+1 for the zero-terminator
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}
//this code used to generate random bytestream
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
	//socket
	int s; 
	int serverPort;
	struct sockaddr_in sin;
	struct sockaddr_in ssin;
	/*Server Buffer*/
    char serverBuf[MAX_BUF];
	/*Payload Size*/
	uint32_t payloadSize;
	/*Count*/
	uint32_t packageCount;
	/*Spacing*/
	uint32_t packageSpacing;

	ssize_t numBytesRcvd;
	
	/*build address data structure*/
	if(argc != 6)
	{
		printf("Error: <hostname> <portnumber> <payload-size> <package-count> <package-spacing>");
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
	
	while(1){
		/*Create UDP socket*/
		if((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	    {
	        printf("Failed to create socket \n");
	        exit(1);
	    } 
	    /*Payload*/ 
		unsigned char *senderRequest;
		//Fill Payload
		senderRequest = gen_bytestream(payloadSize);
		printf("%s \n", senderRequest);
		printf("PackageSize Check: %lu\n", strlen(senderRequest));

		struct timeval start, end;
		/*Sending Payload*/
		//Start Sending
		int i;
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
		//Sending 3 payloads of size 3 bytes to signal end of transmission
		for (int i = 0; i < 3; i++)
		{
			unsigned char a[3] = "123"; //payload of size 3
			if (sendto(s,a,3,0,(struct sockaddr*)&sin, sizeof(sin)) < 0){
					printf("Fail to send\n");
					exit(1);
			}
		}
    	//Done Sending
    	gettimeofday(&end, NULL);
    	float completionTime = (end.tv_sec - start.tv_sec)*1000 + 
               (end.tv_usec - start.tv_usec)/1000.0;
    	fprintf(stdout,"Completion Time: %f ms\n", completionTime);
    	//package divde by completion time
		printf("Package Per Second (PPS): %f packages/s\n", packageCount*1000/completionTime);
		//package count *(header + packageSize*8)
		printf("Bits Per Second (BPS): %f bps\n", ((packageCount*(payloadSize*8+64)+3*(24+64))*1000)/completionTime);
    	exit(0);
	 }
	return 0;
}
