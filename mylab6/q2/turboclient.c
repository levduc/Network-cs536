#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>

#define CLIENT_MAX_BUF 1024
#define MAX_BUF 1024
#define SK_MAX 20

/*concatString is to concatenate two string together*/
char* concatString(char *s1, char *s2)
{
    char *result = malloc(strlen(s1)+strlen(s2)+1); //+1 for the zero-terminator
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}

int main(int argc, char *argv[])
{
    /*Client Buffer*/ 
    char clientBuf[CLIENT_MAX_BUF];
    /*Client Request*/
    char clientRequest[CLIENT_MAX_BUF];
    int s;
    int len;
    /*Client secret key*/ 
    char* c_secret_key; 
    /*filename*/
    char* fileName;
    /*Number of byte for single write*/
    int payloadSize; 
    /*server port*/
    int serverPort;
    /*Build address data structure*/
    /*Check for client input*/
    if(argc != 6)
    {
        printf("Error: <hostname> <portnumber> <secretkey> <filename> <configfile>\n");
        exit(1);
    }
    /***********************build udp-server structure****************/
    struct sockaddr_in serverSin;
    if(inet_pton(AF_INET, argv[1], &serverSin.sin_addr)<=0)
    {
        printf("inet_pton error occured\n");
        exit(1);
    }
    /*Get port number*/
    serverPort = strtol(argv[2],NULL,10); 
    printf("Portnum: %d\n", serverPort);
    /* Address family = Internet */
    serverSin.sin_family = AF_INET;
    /* Set port number, using htons function to use proper byte order */
    serverSin.sin_port = htons(serverPort);
    /* Set all bits of the padding field to 0 */
    memset(serverSin.sin_zero, '\0', sizeof serverSin.sin_zero);
    /***********************build udp-server structure****************/

    /***************Check and Get Client Secret Key*******************/
    if(strlen(argv[3]) < 10)
    {
        printf("Secret key is too short\n");
        exit(1);
    }
    if(strlen(argv[3]) > 20){
        printf("Secret key is too long\n");
        exit(1);
    }
    c_secret_key = argv[3];
    printf("SecretKey: %s\n", c_secret_key);   
    /***************Check and Get Client Secret Key*******************/

    /*****************Checking file name******************************/
    //16 character
    if(strlen(argv[4]) > 16)
    {
        printf("FileName cannot have more than 16 characters\n");
        exit(1);
    }
    //Checking for spacing and forward slash
    int i;
    for (i = 0; i < strlen(argv[4]); ++i)
    {
        if (isspace(argv[4][i]))
        {
            printf("FileName cannot have a space\n");
            exit(1);
        }
        if ((argv[4][i] == '/'))
        {
            printf("FileName cannot have forward slash character\n");
            exit(1);
        }
    }
    fileName = argv[4];
    printf("FileName: %s\n", fileName);
    /*****************Checking file name******************************/

    /*****************Open Configfile*********************************/
    int fdat; 
    if ((fdat = open(argv[5],O_RDWR)) <= 0)
    {
        printf("Fail to open file\n");
        exit(1);
    }
    /*create buffer to read from file*/
    char bsize[MAX_BUF];
    memset(bsize, 0, MAX_BUF);
    if(read(fdat, bsize, MAX_BUF) < 0 )
    {
        printf("Cannot read file\n");
    }
    bsize[MAX_BUF-1] = '\0';
    close(fdat);
    //Convert String to Int
    payloadSize = strtol(bsize, NULL,10);
    printf("payloadSize: %d\n", payloadSize);
    /*****************Open Configfile*********************************/
   
    ssize_t total;
    while(1)
    {
        /**************creating udp-socket****************************/
        if((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        {
            printf("Failed to create socket \n");
            exit(1);
        } 
        /**************creating udp-socket****************************/

        /**********creat and send file request**************************/
        sprintf(clientRequest,"$%s$", c_secret_key);
        //Concatenate with filename
        char * fileRequest = concatString(clientRequest,fileName);
        if(sendto(s,fileRequest,strlen(fileRequest)+1,0
            ,(struct sockaddr*)&serverSin, sizeof(serverSin)) < 0)
        {
            close(s);
            printf("error\n");
            continue;
        }
        //free request
        free(fileRequest);
        /**********creat and send file request**************************/

        /**********waiting for server data******************************/
        /*first 4 bytes are sequence number*/
        char serverBuf[payloadSize+4];
        memset(serverBuf,'\0', payloadSize+4);
        ssize_t numBytesRcvd;
        //file descriptor
        int downFD;
        //total bytes
        total = 0;
        //Measure Time
        struct timeval start, end;
        int firstRead = 1;
        /*waiting for resspone from server*/
        struct sockaddr_in fromServerSin;
        socklen_t sendsize = sizeof(fromServerSin);
        int32_t LAF=1000;
        int32_t LFR=0;
        unsigned char negACK[6];
        unsigned char posACK[6];
        char *arr[1000];
        int i;
        for(i=0;i<1000;i++)
        {
            arr[i]=malloc(1004*sizeof(char)+1);
            memset(arr[i],'\0',1004);
        }
        int lasPacket;
        while((numBytesRcvd = recvfrom(s,serverBuf,payloadSize+4,0,(struct sockaddr *)&fromServerSin,&sendsize))>0) // recv
        {
            /*get sequence number*/
            int temp =  (int)  ((unsigned char)(serverBuf[0]) << 24 |
                               (unsigned char)(serverBuf[1]) << 16 |
                               (unsigned char)(serverBuf[2]) << 8 |
                               (unsigned char)(serverBuf[3]));
            /*package lost sending negative ack*/
            if(temp > LAF)
            {
                continue;
            }

            if((temp > LFR) && (strlen(arr[temp%1000])==0))
            {
                negACK[0] = 'M';
                negACK[1] = ' ';
                negACK[2] = ((LFR) >> 24) & 0xFF;
                negACK[3] = ((LFR) >> 16) & 0xFF;
                negACK[4] = ((LFR) >> 8) & 0xFF;
                negACK[5] = (LFR) & 0xFF;
                memcpy(arr[temp%1000],serverBuf,1004);
                printf("%s\n",arr[temp%1000]);
                if (sendto(s,negACK,6,0,(struct sockaddr*)&fromServerSin, sizeof(fromServerSin)) < 0)
                {
                    printf("Fail to send\n");
                    exit(1);
                }
                printf("negACK sent: %c %x%x%x%x\n",negACK[0],negACK[2],negACK[3],negACK[4],negACK[5]);
            }
            if(LFR == temp)
            {   
           
                if(firstRead == 1)
                {
                    /*Creating file*/
                    downFD = open(fileName, O_CREAT | O_WRONLY | O_EXCL, S_IRUSR | S_IWUSR);
                    if (downFD < 0) 
                    {
                      /*failure: file existed */
                      if (errno == EEXIST) {
                        printf("File already existed\n");
                        close(s);
                        exit(1);
                      }
                    }
                    //Start Time 
                    gettimeofday(&start, NULL);
                    firstRead = 0;
                }
                write(downFD,&serverBuf[4],numBytesRcvd-4);
                total += numBytesRcvd;
                LFR++;
                LAF++;
                while(strlen(arr[LFR%1000]) != 0)
                {
                    write(downFD,&arr[LFR][4],numBytesRcvd-4);
                    memset(arr[LFR],'\0',1004);
                    LFR++;
                    LAF++;
                    total += numBytesRcvd;
                }
            }
            memset(serverBuf, '\0', payloadSize);
        }
        //End Time 
        gettimeofday(&end, NULL);
        close(s);
        /*Printing information*/
        fprintf(stdout,"=======================================\n");
        fprintf(stdout,"\"%s\" is downloaded.\n", fileName);
        fprintf(stdout,"Number of Bytes: %ld bytes\n", total);
        /*Calculate completion time in ms*/
        float t = (end.tv_sec - start.tv_sec)*1000.0 + (end.tv_usec - start.tv_usec)/1000.0;
        fprintf(stdout,"Completion Time: %f ms\n", t);
        fprintf(stdout,"Reliable Throughput: %f bps\n", (total*8*1000)/t);
        exit(0);
     }
    return 0;
}

