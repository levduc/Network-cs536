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


#define CLIENT_MAX_BUF 2048
#define MAX_BUF 1024

int main(int argc, char *argv[])
{
	int status;
	/*Address*/
  	struct sockaddr_in sin;
  	struct sockaddr_in csin;
  	/*socket or fd*/
    int s;
	int vpn_portnumber;
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
        printf("\n Usage: <vpn-portnumber>\n");
        exit(1);
    } 
    //convert to int
    vpn_portnumber = strtol(argv[1],NULL,10);    
    printf("Port Number: %d\n", vpn_portnumber);
    /*build address data structure*/
  	/* Address family = Internet */
    sin.sin_family = AF_INET;
  	/* Set port number, using htons function to use proper byte order */
  	sin.sin_port = htons(vpn_portnumber);
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
	
	while(1)
	{
		/*Clear Buffer*/
		memset(buf,0,CLIENT_MAX_BUF);
		/*Block call*/
		socklen_t sendsize = sizeof(csin);
		if((numBytesRcvd = recvfrom(s, buf, sizeof(buf), 0, (struct sockaddr *) &csin, &sendsize)) < 0)
		{
			exit(1);
		}
		printf("Receive: %s\n", buf);
		/*fork a new child process to handle request*/
		pid_t k = fork(); 
		if(k < 0) /*Fail to create a child process*/
		{
		 	printf("fork() failed!\n");
		}
		else if (k==0) 
		{ 
  			struct sockaddr_in nsin;
			int new_s;
			//Child code
			srand(time(NULL));
			int portNumber = rand()%89999+10000;
		    /*build address data structure*/
		  	/* Address family = Internet */
		    nsin.sin_family = AF_INET;
		  	/* Set port number, using htons function to use proper byte order */
		  	nsin.sin_port = htons(portNumber);
		  	/* Set IP address to localhost */
		  	nsin.sin_addr.s_addr = htonl(INADDR_ANY);
		  	/* Set all bits of the padding field to 0 */
		  	memset(nsin.sin_zero, '\0', sizeof nsin.sin_zero);
		   	/*Creating Socket*/
		   	if ((new_s = socket(AF_INET,SOCK_DGRAM,0)) < 0)
			{
			 	printf("Child:Fail to create socket");
			 	exit(1);
			}
			//Binding 
		   	if ((bind(new_s, (struct sockaddr *)&nsin, sizeof(nsin))) < 0)
		   	{
		   		printf("Fail to bind");
		   		exit(1);
		   	}
		   	/*Response Client with port number*/
			char response[10];
			sprintf(response,"%d", portNumber);
			if (sendto(s,response,strlen(response),0,(struct sockaddr*)&csin, sizeof(csin)) < 0){
				printf("Fail to send\n");
				exit(1);
			}
    	}
	  	else if(k>0) 
	  	{	
			//parent code 
		    //terminate child process
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
