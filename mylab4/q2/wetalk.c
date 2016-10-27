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


#define CLIENT_MAX_BUF 2048
#define MAX_BUF 4096

/*Alarm handler*/
void alarm_handler(int sig_num)
{
    if(sig_num == SIGALRM)
    {
        printf("No response after 7s.\n");
        fprintf(stdout,"?");
        char c = getchar();
        if ( c == 'R' || c == 'r')
        {
            printf("Not sure how to restart\n");
        } 
        else if ( c == 'q' || c == 'Q')
        {
          exit(0);
        } 
        else 
        {
          printf("Invalid character !\n");
          exit(1);
        }
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

int main(int argc, char *argv[])
{
    /*Address*/
    struct sockaddr_in sin;
    struct sockaddr_in csin;
    /*socket*/
    int s;
    /*initator port number*/
    int portNumber;
    /*Number of byte received*/
    ssize_t numBytesRcvd;
    /*buffer*/
    char buf[CLIENT_MAX_BUF];
    /*Child Count*/
    unsigned int childCount = 0;
    /*bool for chat*/
    int chatSession = 0;
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


    /*fork a new child process to handle request*/
    pid_t k = fork(); 
    if(k < 0) /*Fail to create a child process*/
    {
        printf("fork() failed!\n");
    }
    else if (k==0) 
    {
        /*incoming request */
        struct sockaddr_in nsin;
        socklen_t nsendsize = sizeof(nsin);
        /*Clear Buffer*/
        memset(buf,0,MAX_BUF);
        if((numBytesRcvd = recvfrom(s, buf, sizeof(buf), 0, (struct sockaddr *) &nsin, &nsendsize)) < 0)
        {
          printf("[child]: something wrong\n");
          exit(1);
        }
        printf("Incoming request \n");
        printf("\n");
        fgets(buf, 11, stdin);
    }
    else 
    {
        while(chatSession != 0)
        {
            /*Handling User Input*/
            int len;
            char hostName[CLIENT_MAX_BUF];
            /*Get hostname*/
            memset(buf,0,CLIENT_MAX_BUF);
            fprintf(stdout,"?");
            /*read user input*/
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

            /*Create UDP socket*/
            // if((ps = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
            // {
            //     printf("Failed to create socket p\n");
            //     exit(1);
            // }

            /*Sending Request*/
            //Start Sending
            char *payload = "wannatalk";
            if(sendto(s, payload, strlen(payload),0,(struct sockaddr*)&csin, sizeof(csin)) < 0){
              printf("Fail to send\n");
              exit(1);
            }
            /*Set alarm for 7 seconds*/
            alarm(7);
            signal(SIGALRM, alarm_handler);
            //Clean up zombies
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
