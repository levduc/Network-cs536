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
#include <sys/time.h>


#define CLIENT_MAX_BUF 65536
#define MAX_BUF 1024

int main(int argc, char *argv[])
{
	/*Address*/
  	struct sockaddr_in sin;
  	struct sockaddr_in csin;
  	/*socket or fd*/
    int s;
	int serverPort;
	/*Number of byte received*/
    ssize_t numBytesRcvd;
	/*Payload Size*/
	uint32_t payloadSize;
	/*delimtier*/
	const char d[2] = "$"; 
    /*Check user input*/
    if(argc != 3)
    {
        printf("\n Usage: <portnumber> <payload-size>\n");
        exit(1);
    } 
    //convert to int
    serverPort = strtol(argv[1],NULL,10);    
    printf("Port Number: %d\n", serverPort);
    /*Get PayLoad Size*/
    payloadSize = strtol(argv[2],NULL,10); ;
    printf("payloadSize: %d\n", payloadSize);

    /*build address data structure*/
  	/* Address family = Internet */
    sin.sin_family = AF_INET;
  	/* Set port number, using htons function to use proper byte order */
  	sin.sin_port = htons(serverPort);
  	/* Set IP address to localhost */
  	sin.sin_addr.s_addr = htonl(INADDR_ANY);
  	/* Set all bits of the padding field to 0 */
  	memset(sin.sin_zero, '\0', sizeof sin.sin_zero);
   	/*Creating Socket*/
   	if ((s = socket(AF_INET,SOCK_DGRAM,0)) < 0)
	{
	 	printf("Fail to create socket");
	 	exit(1);
	}
	//Binding 
   	if ((bind(s, (struct sockaddr *)&sin, sizeof(sin))) < 0)
   	{
   		printf("Fail to bind");
   		exit(1);
   	}
	printf("Start listening ... \n");
	/*Start Time and End time*/
	struct timeval start, end;
	//Completion Time
	float completionTime;
	/*Set length of buffer to be double payloadsize for safety*/
    char buf[payloadSize*2]; 
    //Number of received package
	int packageCount;
	//Total bytes received
	int totalBytes;
	packageCount = 0;
	/*Clear Buffer*/
	memset(buf,0,payloadSize*2);
	socklen_t sendsize = sizeof(csin);
	while((numBytesRcvd = recvfrom(s, buf, sizeof(buf), 0, (struct sockaddr *) &csin, &sendsize)) > 0)
	{
		
		packageCount++;
		totalBytes += numBytesRcvd;
		if(packageCount == 1)
		{
			//1st time arrive
			printf("First Package arrived.\n");
			//Get Starting time
			gettimeofday(&start, NULL);
		}
		/*Detect ending signal*/
		if (numBytesRcvd == 3)
		{
			//Get Ending Time
			gettimeofday(&end, NULL);
			//Ignore that package
			packageCount--;
			//Ignore those byte
			totalBytes -= numBytesRcvd;
			printf("End of transmission\n");
		    break;
		}

		memset(buf,0,payloadSize*2);
	}
	/*Measure completion time in second*/
	completionTime = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec)/1000000.0;
	/*Printing information*/
	//Package Count
	fprintf(stdout,"Package Count: %d\n", packageCount);
	//Completion Time
    fprintf(stdout,"Completion Time: %f s\n", completionTime);
    /*Calculate total bit sent*/
	/*payloadSize, 8 bytes of UDP header, 20 bytes of IPv4 Header, 27 bytes of Ethernet Frames*/
	//Assuming DIX
	float totalBitPS = (packageCount*(8+20+27)*8+totalBytes*8)/completionTime;
	fprintf(stdout,"Bits received: %d\n", packageCount*(8+20+24)*8+totalBytes*8);
    //PPS
	fprintf(stdout,"Package Per Second (PPS): %f packages/s\n", packageCount/completionTime);
	/*BPS*/
	fprintf(stdout,"Bits Per Second (BPS): %f bps\n", totalBitPS);
    return 0;
}
