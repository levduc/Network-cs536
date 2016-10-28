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
#include <netdb.h>
#include <setjmp.h>
#include <signal.h>
#include <errno.h>


#define CLIENT_MAX_BUF 2048
#define MAX_BUF 4096

/*socket*/
int s;
void SIGIOHandler(int signalType); // Handle SIGIO

/*Alarm handler*/
void signal_handler(int sig_num);


/*Function to convert hostname to ip address*/
int hostname_to_ip(char * hostname , char* ip);

int main(int argc, char *argv[])
{
    /*Address*/
    struct sockaddr_in sin; //self address
    struct sockaddr_in csin;//address of destination 
    struct sockaddr_in nsin;//address of incoming request
    /*initator port number*/
    int portNumber;
    /*Number of byte received*/
    ssize_t numBytesRcvd;
    /*buffer*/
    char buf[CLIENT_MAX_BUF];
    /*Child Count*/
    unsigned int childCount = 0;
    int status;
    /*Check user input*/
    if(argc != 2)
    {
        printf("\n Usage: <portnumber>\n");
        exit(1);
    } 
    //convert to int
    portNumber = strtol(argv[1],NULL,10);    
    /*Build address data structure*/
    /*Address family = Internet */
    sin.sin_family = AF_INET;
    /* Set port number, using htons function to use proper byte order */
    printf("using this %d\n", portNumber);
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

    /*Handling User Input*/
    int chatSession = 0;
    int len;
    const char *invitation = "wannatalk";
    const char *accept = "OK";
    const char *deny = "KO";

    // signal(SIGALRM, signal_handler);   

    /*SigIO handler*/
    struct sigaction handler;
    handler.sa_handler = SIGIOHandler; // Set signal handler for SIGIO
    // Create mask that mask all signals
    if (sigfillset(&handler.sa_mask) < 0)
      printf("sigfillset() failed");
    handler.sa_flags = 0;              // No flags

    if (sigaction(SIGIO, &handler, 0) < 0)
      printf("sigaction() failed for SIGIO");

    // We must own the socket to receive the SIGIO message
    if (fcntl(s, F_SETOWN, getpid()) < 0)
      printf("Unable to set process owner to us");

    // Arrange for nonblocking I/O and SIGIO delivery
    if (fcntl(s, F_SETFL, O_NONBLOCK | FASYNC) < 0)
      printf("Unable to put client sock into non-blocking/async mode");
    // Go off and do real work; echoing happens in the background
 
    while(1)
    {
        if(chatSession == 0)
        { 
            int len;
            char hostName[CLIENT_MAX_BUF];
            /*Get hostname*/
            fprintf(stdout,"?");
            /*read user input*/
            memset(buf,0,CLIENT_MAX_BUF);
            fgets(buf, MAX_BUF, stdin);
            len = strlen(buf);
            buf[len-1] = '\0';
            /*delimiter*/
            const char d[2] = " "; 
            char *token;            
            //Split by delimiter
            token = strtok(buf, d);
            strncpy(hostName, token, strlen(token));
            /*port number*/
            char partnerPort[11];
            memset(partnerPort,0,11);
            token = strtok(NULL, d);
            if(token != NULL)
            {
              strncpy(partnerPort, token, strlen(token));
            }
            else
            {
              printf("There is no port\n");
              exit(1);
            }
            /*hostname to ip*/
            char ip[100];
            len = strlen(hostName);
            hostname_to_ip(hostName, ip);
            /*port number*/
            int partnerPortNumber = strtol(partnerPort,NULL,10);    
            /*Build address data structure of partner*/
            /*Address family = Internet */
            csin.sin_family = AF_INET;
            /* Set port number, using htons function to use proper byte order */
            csin.sin_port = htons(partnerPortNumber);
            /* Set IP address to localhost */
            if(inet_pton(AF_INET, ip, &csin.sin_addr)<=0)
            {
                printf("\n inet_pton error occured\n");
                exit(1);
            }
            /* Set all bits of the padding field to 0 */
            memset(csin.sin_zero, '\0', sizeof csin.sin_zero);
            printf("%s's ip: %s\n", hostName, ip);
            printf("Portnumber: %d \n", partnerPortNumber);

            /*Sending Request*/
            //Start Sending
            if(sendto(s, invitation, strlen(invitation),0,(struct sockaddr*)&csin, sizeof(csin)) < 0){
              printf("Fail to send\n");
              exit(1);
            }
            /*Set alarm for 7 seconds*/
            alarm(7);
        }
        memset(buf,0,MAX_BUF);
        socklen_t sendsize = sizeof(csin);
        if((numBytesRcvd = recvfrom(s, buf, sizeof(buf), 0, (struct sockaddr*)&csin, &sendsize)) < 0)
        {
          printf("Fail to receive\n");
          exit(1);
        }
        /*receive something and not in chatsession*/
        char str[INET_ADDRSTRLEN];
        if(chatSession == 0)
        {
            if(strcmp(invitation,buf) == 0)
            {
                inet_ntop(AF_INET, &(csin.sin_addr), str, INET_ADDRSTRLEN);
                printf("|Chat request from %s %d\n", str, csin.sin_port);
                printf("?[c]hat or [n]o\n");
                char c = getchar();
                if (c == 'c' || c == 'C')
                {
                  char *answer = "OK";
                  if(sendto(s, answer, strlen(answer),0,(struct sockaddr*)&csin, sizeof(csin)) < 0){
                    printf("Fail to send\n");
                    exit(1);
                  }
                  chatSession = 1;
                } 
                else 
                {
                  char *answer = "KO";
                  if(sendto(s, answer, strlen(answer),0,(struct sockaddr*)&csin, sizeof(csin)) < 0){
                    printf("Fail to send\n");
                    exit(1);
                  }
                }
            } 
            if(strcmp(accept,buf) == 0)
            {
                inet_ntop(AF_INET, &(csin.sin_addr), str, INET_ADDRSTRLEN);
                printf("|Chat request is accepted %s %d\n", str, ntohs(csin.sin_port));
                chatSession = 1;
            } 
            if(strcmp(deny,buf) == 0)
            {
                inet_ntop(AF_INET, &(csin.sin_addr), str, INET_ADDRSTRLEN);

                printf("|Doesn't want to chat %s %d\n", str, ntohs(csin.sin_port));
            }
        }
        /*receive something and in chat session*/
        else if(chatSession != 0)
        {
            printf("%s\n", buf);
        }
        if(chatSession != 0)
        {
            printf(">");
            memset(buf,0,CLIENT_MAX_BUF);
            fgets(buf, MAX_BUF, stdin);
            len = strlen(buf);
            buf[len-1] = '\0';
            if(sendto(s, buf, strlen(buf),0,(struct sockaddr*) &csin, sizeof(csin)) < 0)
            {
              printf("Fail to send\n");
              exit(1);
            }
        }
      }
    }
    
void SIGIOHandler(int signalType) {
  ssize_t numBytesRcvd;
  do { // As long as there is input...
    struct sockaddr_storage clntAddr;  // Address of datagram source
    socklen_t clntLen = sizeof(clntAddr); // Address length in-out parameter
    char buffer[MAX_BUF];      // Datagram buffer
    numBytesRcvd = recvfrom(s, buffer, MAX_BUF, 0, (struct sockaddr *) &clntAddr, &clntLen);
    if (numBytesRcvd < 0) {
      // Only acceptable error: recvfrom() would have blocked
      if (errno != EWOULDBLOCK)
        printf("recvfrom() failed");
    } else {
      fprintf(stdout, "Handling client ");
      fprintf(stdout, "%d\n", signalType);
      // PrintSocketAddress((struct sockaddr *) &clntAddr, stdout);
      // fputc('\n', stdout);

      // ssize_t numBytesSent = sendto(s, buffer, numBytesRcvd, 0, (struct sockaddr *) &clntAddr, sizeof(clntAddr));
      //   if (numBytesSent < 0)
      //     printf("sendto() failed");
      //   else if (numBytesSent != numBytesRcvd)
      //     printf("sendto() sent unexpected number of bytes");
    }
  } while (numBytesRcvd > 0);
  // Nothing left to receive
}

/*alarm handler*/
void signal_handler(int sig_num)
{
    if(sig_num == SIGALRM)
    {
        printf("No response after 7s. [q]uit or [r]estart \n");
        fprintf(stdout,"?");
        char c = getchar();
        if ( c == 'R' || c == 'r')
        { 
            printf("Not sure how to restart\n");
        } 
        else if ( c == 'q' || c == 'Q')
        {
          printf("quitting\n");
          exit(0);
        } 
        else 
        {
          printf("Invalid character !\n");
          exit(1);
        }
    }

    if(sig_num == SIGPOLL)
    {
        printf("ahihi\n");
    } 
}

/*Function to convert hostname to ip address*/
/*Reference: http://www.binarytides.com/hostname-to-ip-address-c-sockets-linux/*/
int hostname_to_ip(char * hostname , char* ip)
{
    struct hostent *he;
    struct in_addr **addr_list;
    int i;    
    if ( (he = gethostbyname( hostname ) ) == NULL) 
    {
        // get the host info
        herror("gethostbyname");
        exit(1);
    }
    addr_list = (struct in_addr **) he->h_addr_list;
    for(i = 0; addr_list[i] != NULL; i++) 
    {
        //Return the first one;
        strcpy(ip , inet_ntoa(*addr_list[i]) );
        return 0;
    }    
    exit(1);
}    
