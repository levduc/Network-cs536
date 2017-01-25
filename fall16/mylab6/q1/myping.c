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
#define SK_MAX 20
//used to concatenate two string
//for alarm

char* concatString(char *s1, char *s2)
{
    char *result = malloc(strlen(s1)+strlen(s2)+1);//+1 for the zero-terminator
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}
//this code used to generate random bytestream
char *gen_bytestream (size_t num_bytes)
{
  static const char AllChar[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
  srand(time(NULL));
  char *bytestream = malloc (num_bytes);
  size_t i;
  for (i = 0; i < num_bytes; i++)
  {
    bytestream[i] = AllChar[rand() % (sizeof(AllChar) - 1)];
  }
  return bytestream;
}
//Signal Handler
int count = 0;
void signal_handler(int sig_num)
{
    if(sig_num == SIGALRM)
    {
    	signal(SIGALRM, signal_handler); /* reinstall the handler */
    	count++;
    }
    if(sig_num == SIGALRM && (count == 3) ) //count == 31 mean 1.55 second elapsed after first signal
    {
    	printf("No response from ping server after 2.55 seconds\n");
    	exit(1);
    }
}

int main(int argc, char *argv[])
{

	// buf
	int s; //socket
	ssize_t len;
	int serverPort;
	char* c_secret_key; // client secret key
	struct sockaddr_in sin;
	struct sockaddr_in ssin;
    char serverBuf[MAX_BUF];
    char cfifopid[CLIENT_MAX_BUF];
	char clientRequest[CLIENT_MAX_BUF];
	ssize_t numBytesRcvd;
	/*build address data structure*/
	if(argc != 5)
	{
		printf("Error: <hostname> <portnumber> <secretkey> <dedicated-port>");
		exit(1);
	}
	else
	{
  		/* Set IP address to localhost */
	  	if(inet_pton(AF_INET, argv[1], &sin.sin_addr)<=0)
	    {
	        printf("\n inet_pton error occured\n");
	        exit(1);
	    }
	    //convert to int
	    serverPort = strtol(argv[2],NULL,10);
	    c_secret_key = argv[3];
	    c_secret_key[SK_MAX] = '\0';
	}
  	/* Address family = Internet */
    sin.sin_family = AF_INET;
  	/* Set port number, using htons function to use proper byte order */
  	sin.sin_port = htons(serverPort);
  	/* Set all bits of the padding field to 0 */
  	memset(sin.sin_zero, '\0', sizeof sin.sin_zero);
	
    //half association
	while(1){
	    struct sockaddr_in thisSin;
	    /*binding*/
	  	/* Address family = Internet */
	    thisSin.sin_family = AF_INET;
	  	/* Set port number, using htons function to use proper byte order */
	    int16_t thisPort;
	    thisPort = strtol(argv[4],NULL,10);
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
	    sprintf(clientRequest,"$%s$", c_secret_key);
		len = strlen(clientRequest);
		clientRequest[len] = '\0';
		char *clientBuf;
		size_t ditmemayc = (size_t) (1000 - len);
		clientBuf = gen_bytestream(ditmemayc);
		//concatenate
		char * IDRequest = concatString(clientRequest,clientBuf);
		len = strlen(IDRequest);
		IDRequest[len] = '\0';
		struct timeval start, end;
		gettimeofday(&start, NULL);
		if (sendto(s,IDRequest,strlen(IDRequest),0, (struct sockaddr*)&sin, sizeof(sin)) < 0){
			printf("Fail to send\n");
			exit(1);
		}
		printf("Length: %lu\n", strlen(IDRequest));
		printf("Request Sent... \n");
		// printf("%s \n", IDRequest);
		//free request
		free(IDRequest);
		free(clientBuf);
		// alarm
    	signal(SIGALRM, signal_handler);
    	ualarm(950000,800000); //after 950000 ms we need 950,000 + 2*800,000 = 2,550,000
		//receive from server
		socklen_t sendsize = sizeof(ssin);
		if((numBytesRcvd = recvfrom(s, serverBuf, sizeof(serverBuf), 0, (struct sockaddr*)&ssin, &sendsize)) < 0) // recv
    	{
    		perror("recvfrom() failed");
    		exit(1);
    	}
    	serverBuf[numBytesRcvd] = '\0';
    	//set alarm 
		// struct sigaction act;
    	//Received
    	gettimeofday(&end, NULL);
    	printf("Server Response: %s\n", serverBuf);
    	fprintf(stdout,"Time Elapsed: %f ms\n", (end.tv_sec - start.tv_sec)*1000 + 
              ((end.tv_usec - start.tv_usec)/1000.0));
    	fprintf(stdout,"Server's Port Number: %d\n", ntohs(ssin.sin_port));
    	//convert ip to string
    	char str[INET_ADDRSTRLEN];
    	inet_ntop(AF_INET, &(ssin.sin_addr), str, INET_ADDRSTRLEN);
		fprintf(stdout,"Server's IP Address: %s\n", str);
    	//close socket
    	close(s);
	    memset(clientRequest,0, CLIENT_MAX_BUF);
	    memset(cfifopid,0, CLIENT_MAX_BUF);
	    memset(serverBuf,0, CLIENT_MAX_BUF);
    	exit(0);
	 }
	return 0;
}
