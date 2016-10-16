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
    int serverFDPID; // server file description
	const char d[2] = "$"; //delimtier
  	struct sockaddr_in sin;
  	struct sockaddr_in csin;
    int s, new_s; //socket or fd
    ssize_t numBytesRcvd;
	int serverPort;
    /*Check user input*/
    if(argc != 2)
    {
        printf("\n Usage: <portnumber>\n");
        exit(1);
    } 
    //convert to int
    serverPort = strtol(argv[1],NULL,10);    
    printf("Port Number: %d\n", serverPort);
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
   	if ((bind(s, (struct sockaddr *)&sin, sizeof(sin))) < 0)
   	{
   		printf("Fail to bind");
   		exit(1);
   	}
	printf("Start listening ... \n");
	struct timeval start, end;
	float completionTime;
    char buf[CLIENT_MAX_BUF];
	int endCount;
	int packageCount;
   	int firstPackage;
	int totalBytes;
	memset(buf,0,CLIENT_MAX_BUF);
	packageCount = 0;
	endCount = 0;
	socklen_t sendsize = sizeof(csin);
	while((numBytesRcvd = recvfrom(s, buf, sizeof(buf), 0, (struct sockaddr *) &csin, &sendsize)) > 0)
	{
		
		packageCount++;
		totalBytes += numBytesRcvd;
		memset(buf,0,CLIENT_MAX_BUF);
		// printf("%ld\n", numBytesRcvd);
		if(packageCount == 1)
		{
			//1st time arrive
			printf("First Package arrived.\n");
			gettimeofday(&start, NULL);
		}
		if (numBytesRcvd == 3)
		{
			packageCount--;
			totalBytes -= numBytesRcvd;
			gettimeofday(&end, NULL);
			printf("End of transmission\n");
		    break;
		}
	}
	completionTime = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec)/1000000.0;
	printf("Package Count: %d\n", packageCount);
    fprintf(stdout,"Completion Time: %f s\n", completionTime);
    /*PPS*/
	printf("Package Per Second (PPS): %f packages/s\n", packageCount/completionTime);
    /*Calculate total bit sent*/
	/*payloadSize, 8 bytes of UDP header, 20 bytes of IPv4 Header, 24 bytes of Ethernet Frames*/
	float totalBitPS = (packageCount*(8+20+24)*8+totalBytes*8)/completionTime;
	printf("totalbit: %d\n", packageCount*(8+20+24)*8+totalBytes*8);
	/*BPS*/
	printf("Bits Per Second (BPS): %f bps\n", totalBitPS);
    return 0;
}
