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

/*concate two string function*/
char* concatString(char *s1, char *s2)
{
    char *result = malloc(strlen(s1)+strlen(s2)+1);//+1 for the zero-terminator
    strcpy(result, s1);
    strcat(result, s2);
    return result;
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
  	sin.sin_addr.s_addr = htonl(INADDR_ANY);

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
	
   struct ifaddrs *ifap, *ifa;
   struct sockaddr_in *sa;
   char *addr;
   getifaddrs (&ifap);
   for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
       if (ifa->ifa_addr->sa_family==AF_INET) {
           sa = (struct sockaddr_in *) ifa->ifa_addr;
           addr = inet_ntoa(sa->sin_addr);
           printf("Interface: %s\tAddress: %s\n", ifa->ifa_name, addr);
       }
   }
   freeifaddrs(ifap);

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
			/* breakdown mytunnel request*/
			printf("child [%d]:\n", getpid());
			char* token;
			char buildRequest[16];
			/* split by delimiter*/
		    token = strtok(buf, d);
		    strncpy(buildRequest, token, 15);
			buildRequest[16] = '\0';
			char currentIP[16];
			while(token != NULL)
			{
				strncpy(currentIP, token, 15);
				currentIP[16] = '\0';
		    	token = strtok(NULL, d);
			}
			printf("last ip: %s\n", currentIP); 
			
			/* compare with local address*/

		    /* build server address data structure*/
  	
  	// 		struct sockaddr_in nsin;
			// int serverSock;
   //  		ssize_t bytesRcvd;
			// int s_port;
		 //  	/*Address family = Internet */
		 //    nsin.sin_family = AF_INET;
		 //    /*IP Address*/
		 //    if(inet_pton(AF_INET, sIpAddress, &nsin.sin_addr)<=0)
		 //    {
		 //        printf("Child: inet_pton error occured\n");
		 //        exit(1);
		 //    }
		 //  	/* Set port number, using htons function to use proper byte order */
   // 			s_port = strtol(sPort,NULL,10);
   // 			printf("server port %d\n", s_port);
		 //  	nsin.sin_port = htons(s_port);
		 //  	/* Set all bits of the padding field to 0 */
		 //  	memset(nsin.sin_zero, '\0', sizeof nsin.sin_zero);
		 //   	/*Creating Socket*/
		 //   	if ((serverSock = socket(AF_INET,SOCK_DGRAM,0)) < 0)
			// {
			//  	printf("Child:Fail to create socket");
			//  	exit(1);
			// }
			// /*tunneld new address data structure to forward*/
  	// 		struct sockaddr_in nssin;
			// int clientSock;
			// /*Get random port number between 10001 and 99999*/
			// srand(time(NULL));
			// int portNumber = rand()%89999+10001;
			// char response[10];
		 //    /*build address data structure*/
		 //  	/* Address family = Internet */
		 //    nssin.sin_family = AF_INET;
		 //  	/* Set port number, using htons function to use proper byte order */
		 //  	nssin.sin_port = htons(portNumber);
		 //  	/* Set IP address to localhost */
		 //  	nssin.sin_addr.s_addr = htonl(INADDR_ANY);
		 //  	/* Set all bits of the padding field to 0 */
		 //  	memset(sin.sin_zero, '\0', sizeof sin.sin_zero);
   // 			/*Creating Socket*/
		 //   	if ((clientSock = socket(AF_INET,SOCK_DGRAM,0)) < 0)
			// {
			//  	printf("Child: Fail to create socket");
			//  	exit(1);
			// }
			// //Binding 
		 //   	if ((bind(clientSock, (struct sockaddr *)&nssin, sizeof(nssin))) < 0)
		 //   	{
		 //   		printf("Child: Fail to bind");
		 //   		exit(1);
		 //   	}
		 //   	/*Response Client with port number*/
			// sprintf(response,"%d", portNumber);
			// if (sendto(s,response,strlen(response),0,(struct sockaddr*)&csin, sizeof(csin)) < 0){
			// 	printf("Child: Fail to send\n");
			// 	exit(1);
			// }
			// printf("Child is waiting\n");
  	// 		struct sockaddr_in snd_in;
			// /*buffer*/
			// char snd_buf[MAX_BUF];
			// memset(snd_buf,0,MAX_BUF);
			// socklen_t send_size = sizeof(snd_in);
			// /*==============================================Test Lab 3===========================================================*/
			// /*This is for handle traffic generator in lab3*/
			// /*Uncomment to test lab 3 code*/
			// /*Block call*/
			// while((bytesRcvd = recvfrom(clientSock, snd_buf, sizeof(snd_buf), 0, (struct sockaddr *) &snd_in, &send_size)) > 0)
			// {
			// 	/*Forward to server address nsin*/
			// 	if (sendto(serverSock,snd_buf,strlen(snd_buf),0,(struct sockaddr*)&nsin, sizeof(nsin)) < 0){
			// 		printf("Child: Fail to send\n");
			// 		exit(1);
			// 	}
			// 	/*End of transmission package*/
			// 	if (bytesRcvd == 3)
			// 	{
			// 	    break;
			// 	}
			// 	memset(snd_buf,0,MAX_BUF);
			// }
			/*==============================================Test Lab 3===========================================================*/


			/*==============================================Test Lab 2=========================================================*/
			/* this is for myping/mypingd in lab2*/
			/* uncomment to test lab2 code and comment lab3 code*/			
			// memset(snd_buf,0,MAX_BUF);
			// if ((bytesRcvd = recvfrom(clientSock, snd_buf, sizeof(snd_buf), 0, (struct sockaddr *) &snd_in, &send_size)) < 0)
			// {
			// 	printf("Child: Cannot receive package from client\n");
			// 	exit(1);
			// }
			// /* forward to server address nsin*/
			// if (sendto(serverSock,snd_buf,strlen(snd_buf),0,(struct sockaddr*)&nsin, sizeof(nsin)) < 0){
			// 	printf("Child: Fail to send\n");
			// 	exit(1);
			// }
			// memset(snd_buf,0,MAX_BUF);
			// socklen_t sssize = sizeof(nsin);
			// if ((bytesRcvd = recvfrom(serverSock, snd_buf, sizeof(snd_buf), 0, (struct sockaddr *) &nsin, &sssize)) < 0)
			// {
			// 	printf("Child: Cannot receive package from client\n");
			// 	exit(1);
			// }
			// /* forward "terve" to client address snd*/
			// if (sendto(clientSock,snd_buf,strlen(snd_buf),0,(struct sockaddr*)&snd_in, sizeof(snd_in)) < 0){
			// 	printf("Child: Fail to send\n");
			// 	exit(1);
			// }
			/*==============================================Test Lab 2=========================================================*/
			
			printf("Child is done! \n");
			/* done sending kill child process*/
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
