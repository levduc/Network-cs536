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


#define CLIENT_MAX_BUF 2048
#define MAX_BUF 4096

int main(int argc, char *argv[])
{
	int status;
	/*Address*/
  	struct sockaddr_in sin;
  	struct sockaddr_in csin;
  	/*socket or fd*/
    int s;
	int portNumber;
	/*Number of byte received*/
    ssize_t numBytesRcvd;
	/*delimtier*/
	const char d[2] = "$"; 
	/*buffer*/
    char buf[CLIENT_MAX_BUF];
    /*Child Count*/
  	unsigned int childCount = 0;

    /*Check user input*/
    if(argc != 2)
    {
        printf("\n Usage: <portnumber>\n");
        exit(1);
    } 
    //convert to int
    portNumber = strtol(argv[1],NULL,10);    
    /*build address data structure*/
  	/* Address family = Internet */
    sin.sin_family = AF_INET;
  	/* Set port number, using htons function to use proper byte order */
  	sin.sin_port = htons(portNumber);
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
	
	while(1)
	{
		/*Clear Buffer*/
		memset(buf,0,MAX_BUF);
		fprintf(stdout,"?");
		fgets(buf, MAX_BUF, stdin);
		printf("%s\n", buf);
		/*Block call*/
		// socklen_t sendsize = sizeof(csin);
		// if((numBytesRcvd = recvfrom(s, buf, sizeof(buf), 0, (struct sockaddr *) &csin, &sendsize)) < 0)
		// {
			// exit(1);
		// }
		// printf("Receive: %s\n", buf);
	}
    return 0;
}
