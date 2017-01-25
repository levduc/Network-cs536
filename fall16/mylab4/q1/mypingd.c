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

#define CLIENT_MAX_BUF 1024
#define MAX_BUF 1024
#define SK_MAX 20
int main(int argc, char *argv[])
{
	int status;
    int serverFDPID; // server file description
	char c_secret_key[SK_MAX+1]; // client secret key
	char* s_secret_key; // server secret key
	char command[CLIENT_MAX_BUF+1];
    char cfifopid[CLIENT_MAX_BUF];
	const char d[2] = "$"; //delimtier
  	unsigned int childCount = 0; //child count
  	struct sockaddr_in sin;
  	struct sockaddr_in csin;
    char buf[CLIENT_MAX_BUF];
    int s, new_s; //socket or fd
    int len;
    ssize_t numBytesRcvd;
	int serverPort;
    //checking
    if(argc != 3)
    {
        printf("\n Usage: <portnumber> <secretkey>\n");
        exit(1);
    } 
    
    if(strlen(argv[2]) < 10){
    	printf("Secret key is too short\n");
        exit(1);
    }

    if(strlen(argv[2]) > 20){
    	printf("Secret key is too long\n");
        exit(1);
    }

    //convert to int
    serverPort = strtol(argv[1],NULL,10);
    s_secret_key = argv[2];
    s_secret_key[SK_MAX] = '\0';
    printf("%d\n", serverPort);
    /*build address data structure*/
  	/* Address family = Internet */
    sin.sin_family = AF_INET;
  	/* Set port number, using htons function to use proper byte order */
  	sin.sin_port = htons(serverPort);
  	/* Set IP address to localhost */
  	sin.sin_addr.s_addr = htonl(INADDR_ANY);
  	/* Set all bits of the padding field to 0 */
  	memset(sin.sin_zero, '\0', sizeof sin.sin_zero);
   	
   	if ((s = socket(AF_INET,SOCK_DGRAM,0)) < 0) //Check creating UDP
   	 {
   	 	perror("Fail to create socket");
   	 	exit(1);
   	 } 
   	if ((bind(s, (struct sockaddr *)&sin, sizeof(sin))) < 0)
   	{
   		perror("Fail to bind");
   		exit(1);
   	}
   	// listen(s, 10);
	printf("Start listening ... \n");
	int count = 0;
	while(1) //listening
	{
		//clear buffer
		memset(buf,0,CLIENT_MAX_BUF);
		socklen_t sendsize = sizeof(csin);
		if ((numBytesRcvd = recvfrom(s, buf, sizeof(buf), 0, (struct sockaddr *) &csin, &sendsize)) < 0) // recv
		{
			// perror("recvfrom() failed");
			// exit(1);
			//continue;
		}
    	buf[numBytesRcvd] = '\0';
    	//
    	count++;
    	if(count%4 == 0)
    	{
    		printf("Ignore this request\n");
    		continue;
    	}		
		// there is a client request
	    //get the first token
	    char *token;
	    token = strtok(buf, d); 
		//get secret key
		strncpy(c_secret_key, token, SK_MAX+1);
		c_secret_key[SK_MAX] = '\0';
	    token = strtok(NULL, d);
		if(token != NULL)
		{
			strncpy(command, token, CLIENT_MAX_BUF+1);
			command[CLIENT_MAX_BUF] = '\0';
		}
		else
		{
			command[CLIENT_MAX_BUF] = '\0';
		}
		while(token != NULL)
		{
			token = strtok(NULL, d);
		}
	    // printf("Comparing secret key\n");
		//sleep to check
	    //sleep(2);
	    if(strcmp(s_secret_key, c_secret_key) != 0 ) //comparing secret key
	    {
	    	printf("Secret key not match\n");
	    }
	    else if (strlen(command)+strlen(c_secret_key) != 998)
	    {
	    	printf("Length not match\n");
	    }
	    else 
		{
			char *payload = "terve";
	    	if (sendto(s,payload,strlen(payload),0, (struct sockaddr*)&csin, sizeof(csin)) < 0){
				printf("Fail to send\n");
			}
			// printf("%lu\n", strlen(command));
			// printf("%lu\n", strlen(c_secret_key));
		   	// exit(1);
		}
	    memset(c_secret_key,0, CLIENT_MAX_BUF);
	    // memset(s_secret_key,0, CLIENT_MAX_BUF);
	    memset(command,0, CLIENT_MAX_BUF);
	    memset(cfifopid,0, CLIENT_MAX_BUF);
		memset(buf,0,CLIENT_MAX_BUF);
	}
    return 0;
}
