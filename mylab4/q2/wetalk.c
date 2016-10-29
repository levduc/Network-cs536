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

/*incoming*/
/*global socket*/
int s;

char peerHost[MAX_BUF];
size_t peerPort;

const char *invitation = "wannatalk";
const char *peerAccept = "OK";
const char *peerDeny = "KO";
int chatSession = 0;

/*Split invitation request hostname and portnumber*/
int SplitInvitaionRequest(char *hostName, char *portNumber, char *buf);
/*Send wannatalk to peer*/
int SendInvitation(char* hostname, char *portnumber);
/* Handle SIGIO*/
void SIGIOHandler(int sig_num); 
/*Alarm handler*/
void SIGALARM_handler(int sig_num);
/*Function to convert hostname to ip address*/
int hostname_to_ip(char * hostname , char* ip);

int main(int argc, char *argv[])
{
    /*Address*/
    struct sockaddr_in sin; //self address
    struct sockaddr_in csin;//address of destination 
    /*initator port number*/
    int portNumber;
    /*Number of byte received*/
    ssize_t numBytesRcvd;
    
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
    
    char buf[MAX_BUF];
    char hostName[MAX_BUF];
    char partnerPort[MAX_BUF];
    int isInitator = 0;
    

    while(1)
    {
        /*While not in chatSession wait for user input or invitation*/
        while(chatSession == 0)
        { 
            int len = 0;
            memset(buf, 0, MAX_BUF);
            memset(hostName, 0, MAX_BUF);
            memset(partnerPort, 0, MAX_BUF);
            /*Get hostname*/
            printf("?");
            /*read user input*/
            fflush(stdout);
            char n;
            while(((n = getchar()) != '\n') && n != EOF && len < 50 )
            { 
                buf[len] = n;
                len++;
            }
            /*len <= 2 implies response to invitation*/
            if(strlen(buf) < 2)
            {   
                if (buf[0] == 'q' || buf[0] == 'Q')
                {
                  printf("You decide to quit\n");
                  exit(0);
                } 
                else if (buf[0] == 'r' || buf[0] == 'R')
                {
                  printf("Restart\n");
                  continue;
                }
                else 
                {
                  printf("\n");
                  continue;
                }

            } 
            else
            {
                buf[len] = '\0'; //get rid of newline
                /*Splitting buffer*/
                SplitInvitaionRequest(hostName, partnerPort, buf);
                /*Printing information to check*/
                // printf("%s's ip: %s\n", hostName, ip);
                printf("Hostname: %s \n", hostName);
                printf("Portnumber: %s \n", partnerPort);
                printf("Sending Invitaion\n");        
                /*Send invitation*/
                if (SendInvitation(hostName, partnerPort) < 0)
                {
                  fprintf(stdout, "Fail to send invitation\n");
                }
                /*Set alarm for 7 seconds*/
                alarm(7);
                signal(SIGALRM, SIGALARM_handler);
            }
        }
        if (chatSession == 1)
        {
            printf("Connection is established: \n");
            printf("peer ip: %s\n", peerHost);
            printf("peer port: %ld\n", peerPort);
            int len;
            /*hostname to ip*/
            char ip[100];
            len = strlen(peerHost);        
            if (hostname_to_ip(peerHost, ip) < 0)
            {
              printf("Fail to convert hostName to IP \n");
            }  
            /*Build address data structure of partner*/
            /*Address family = Internet */
            csin.sin_family = AF_INET;
            /* Set port number, using htons function to use proper byte order */
            csin.sin_port = htons(peerPort);
            /* Set IP address to localhost */
            if(inet_pton(AF_INET, ip, &csin.sin_addr)<=0)
            {
                printf("\n inet_pton error occured\n");
                return -1;
            }
            /* Set all bits of the padding field to 0 */
            memset(csin.sin_zero, '\0', sizeof csin.sin_zero);
            /*Sending Request*/
        }
        /*During chat session*/
        while(chatSession)
        {
            fflush(stdin);
            memset(buf,0,MAX_BUF);
            int len = 0;
            printf(">");
            char n;
            /*block here*/
            while((n = getchar()) != '\n' && n != EOF)
            {
                buf[len] = n;
                len++;
            }
            if(len == 1 && buf[0] == 'e')
            {
              char realBuf[1] = "E";
              if(sendto(s, realBuf,1,0,(struct sockaddr*) &csin, sizeof(csin)) < 0)
              {
                printf("Fail to send\n");
                exit(1);
              }
              printf("You terminate chat session\n");
              chatSession = 0;          
              break;
            }
            else if(len >= 1)
            {
              char realBuf[52];
              int i;
              realBuf[0] = 'D';
              for(i = 1; i < 52; i++)
              { 
                realBuf[i] = buf[i-1];
              }
              realBuf[52] = '\0';
              if(sendto(s, realBuf, 51,0,(struct sockaddr*) &csin, sizeof(csin)) < 0)
              {
                printf("Fail to send\n");
                exit(1);
              }
            }
        } //endwhile
    }
}
  
void SIGIOHandler(int sig_num) 
{
  ssize_t numBytesRcvd;
  do 
  { 
    struct sockaddr_in csin; //self address
    socklen_t sendsize = sizeof(csin); // Address length in-out parameter
    char buf[MAX_BUF];      // buf
    memset(buf,0,MAX_BUF);
    numBytesRcvd = recvfrom(s, buf, 51, 0, (struct sockaddr *) &csin, &sendsize);
    if (numBytesRcvd < 0) {
      if (errno != EWOULDBLOCK)
        printf("recvfrom() failed");
    } 
    else 
    {
      /*receive something cancel alarm*/
      alarm(0);
      /*ip*/
      char str[INET_ADDRSTRLEN];
      /*Receive something and currently not in chat session*/
      if(chatSession == 0)
      {
          /*Receive invitation*/
          if(strcmp(invitation,buf) == 0)
          {
              inet_ntop(AF_INET, &(csin.sin_addr), str, INET_ADDRSTRLEN);
              printf("|Chat request from %s %d\n", str, ntohs(csin.sin_port));
              printf("[c]hat or [n]o?");
              char c = getchar();
              if (c == 'c' || c == 'C')
              {
                char *answer = "OK";
                if(sendto(s, answer, strlen(answer),0,(struct sockaddr*)&csin, sizeof(csin)) < 0){
                  printf("Fail to send\n");
                  exit(1);
                }
                /*copy peer info to global*/
                chatSession = 1;
                strncpy(peerHost, str, strlen(str));
                peerHost[strlen(str)] = '\0';
                peerPort = ntohs(csin.sin_port);
              } 
              else 
              {
                char *answer = "KO";
                if(sendto(s, answer, strlen(answer),0,(struct sockaddr*)&csin, sizeof(csin)) < 0)
                {
                  printf("Fail to send\n");
                  exit(1);
                }
                chatSession = 0; 
              }
          }
          /*receive peer's confirmation*/
          if(strcmp(peerAccept,buf) == 0)
          {
              inet_ntop(AF_INET, &(csin.sin_addr), str, INET_ADDRSTRLEN);
              printf("|Chat request is accepted %s %d.\n", str, ntohs(csin.sin_port));
              chatSession = 1;
              /*copy peer info to global*/
              strncpy(peerHost, str, strlen(str));
              peerHost[strlen(str)] = '\0';  
              peerPort = ntohs(csin.sin_port);
              // longjmp(JumpBuffer1,1);
          } 
          /*receive peer's deny*/
          if(strcmp(peerDeny,buf) == 0)
          {
              inet_ntop(AF_INET, &(csin.sin_addr), str, INET_ADDRSTRLEN);
              printf("|Doesn't want to chat %s %d. Type 'r' to restart\n", str, ntohs(csin.sin_port));
              // longjmp(JumpBuffer1,1);
          }
      } /*end handshake*/
      /*During chat session. */
      else if(chatSession == 1 && buf[0] == 'D') /*this is a message*/
      {
          printf("|%s\n", &buf[1]);
      } 
      else if (chatSession == 1 && buf[0] == 'E') /*this is the end*/
      {
          printf("|chat terminated. Press 'r' to continue\n");
          chatSession = 0;
          // longjmp(JumpBuffer1,1);
      } 
    }
  } while (numBytesRcvd >= 0);
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
        return -1;
    }
    addr_list = (struct in_addr **) he->h_addr_list;
    for(i = 0; addr_list[i] != NULL; i++) 
    {
        //Return the first one;
        strcpy(ip , inet_ntoa(*addr_list[i]) );
        return 0;
    }    
    return -1;
} 
/*send invitation to chat*/
int SendInvitation(char* hostName, char *partnerPort)
{
    struct sockaddr_in csin;//address of destination 
    int len;
    /*hostname to ip*/
    char ip[100];
    len = strlen(hostName);
    if (hostname_to_ip(hostName, ip) < 0)
    {
      printf("Fail to convert hostName to IP \n");
      return -1;
    }  
    /*Build address data structure of partner*/
    /*Address family = Internet */
    csin.sin_family = AF_INET;
    /* Set port number, using htons function to use proper byte order */
    int partnerPortNumber = strtol(partnerPort,NULL,10);    
    csin.sin_port = htons(partnerPortNumber);
    /* Set IP address to localhost */
    if(inet_pton(AF_INET, ip, &csin.sin_addr)<=0)
    {
        printf("\n inet_pton error occured\n");
        return -1;
    }
    /* Set all bits of the padding field to 0 */
    memset(csin.sin_zero, '\0', sizeof csin.sin_zero);
    /*Sending Request*/
    //Start Sending
    if(sendto(s,invitation, strlen(invitation),0,(struct sockaddr*)&csin, sizeof(csin)) < 0)
    {
      printf("Fail to send\n");
      return -1;
    }
    return 0;
}
/*Splitting user input*/
int SplitInvitaionRequest(char *hostName, char *portNumber, char *buf)
{
    /*delimiter*/
    char *token;            
    const char d[2] =" "; 
    token = strtok(buf, d);
    int len;
    //Split by delimiter
    len = strlen(token);
    strncpy(hostName, token, len);
    hostName[len] = '\0';
    /*port umber*/
    //memset(portNumber,0,11);
    token = strtok(NULL, d);
    if(token != NULL)
    {
      len = strlen(token);
      strncpy(portNumber, token, len);
      portNumber[len] = '\0';
    }
    else
    {
      printf("There is no port\n");
      return -1;
    }
    return 0;
}
/*Alarm handler*/
void SIGALARM_handler(int sig_num)
{
    if(sig_num == SIGALRM)
    {
        printf("No response after 7s. [q]uit or [r]estart \n");
        // char c = getchar();
        // if ( c == 'R' || c == 'r')
        // { 
        //     chatSession = 0;
        //     // int len=0;
        //     // char buf[MAX_BUF];
        //     // char hostName[MAX_BUF];
        //     // char partnerPort[MAX_BUF];
        //     // // /*Get hostname*/
        //     // printf("?");
        //     // /*read user input*/
        //     // memset(buf,0,MAX_BUF);
        //     // char n = getchar();
        //     // while(((n = getchar()) != '\n') && n != EOF && len < 50)
        //     // { 
        //     //   buf[len] = n;
        //     //   len++;
        //     // }
        //     // buf[len] = '\0';
        //     // // /*Splitting buffer*/
        //     // SplitInvitaionRequest(hostName, partnerPort, buf);
        //     // // /*Printing information to check*/
        //     // printf("Hostname: %s \n", hostName);
        //     // printf("Portnumber: %s \n", partnerPort);
        //     // printf("Sending Invitaion\n");
        //     // /*Send invitation*/
        //     // if (SendInvitation(hostName, partnerPort) < 0)
        //     // {
        //     //   fprintf(stdout, "Fail to send invitation\n");
        //     // }
        //     // /*Set alarm for 7 seconds*/
        //     // alarm(7);
        //     // signal(SIGALRM, SIGALARM_handler);
        // } 
        // else if (c == 'q' || c == 'Q')
        // {
        //   printf("Quitting\n");
        //   exit(0);
        // } 
        // else 
        // {
        //   printf("Invalid character !\n");
        //   exit(1);
        // }
    }
}


