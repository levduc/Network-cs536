/* fileserver.c
* @author Hai Nguyen
*/

// tcp server

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//Generic socket:

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

//Generic system:

#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/signal.h>

//Protocol specific socket:

#include <netinet/tcp.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>

#define BUFSIZE 256


void handle_single_client(int sock, char * key, struct sockaddr_in cli_add, char * arg3);

int main(int argc, char * argv[]) {
  int sd, newsd;
  socklen_t len;
  int port_num;
  struct sockaddr_in serv_add, cli_add;
  int pid;

  // Check if the number of arguments is correct
  if (argc != 4) {
    printf("Usage: fileserver portnumber secretkey configfile.dat\n");
    exit(0);
  }

  // Check length of secret key
  len = strlen(argv[2]);
  if (len <10 || len > 20) {
    printf("ERROR secretkey too short or too long\n");
    exit(1);
  }

  // 
  sd = socket(AF_INET, SOCK_STREAM, 0);
  
  if (sd < 0) {
    printf("ERROR opening socket\n");
    exit(1);
  }
   
  // Initilaize socket struct
  port_num = atoi(argv[1]);
  bzero((char *) &serv_add, sizeof(serv_add));

  serv_add.sin_family = AF_INET;
  serv_add.sin_port = htons(port_num);
  serv_add.sin_addr.s_addr = INADDR_ANY;

  // Bind the host address using bind() 
  if (bind(sd, (struct sockaddr *) &serv_add, sizeof(serv_add)) < 0){
    printf("ERROR on binding\n");  
    exit(1);
  }

  // Start listening for the clients
  listen(sd, 5);
  
  len = sizeof(cli_add);
  while(1) {
    //printf("Waiting for data...");
    newsd = accept(sd, (struct sockaddr *) &cli_add, &len);
    if (newsd < 0) {
      printf("ERROR on accept\n");
    }
    
    // Create child process
    pid = fork();

    if (pid <0) {
       printf("ERROR on fork\n");
       exit(1);
    }
   
    if (pid == 0) { // Child process 
      handle_single_client(newsd, argv[2], cli_add, argv[3]);
    }
    else { // Parent process
       close(newsd);
       printf("***************************************\n");
    }
  }

  close(sd);
}

/* Function for handling a single client request */
void handle_single_client(int sock, char * key, struct sockaddr_in cli_add, char * arg3){
  int n, fd, blocksize;
  int bytes_read, bytes_write;
  char * filename;
  char * secretkey;
  char buffer[BUFSIZE];
  bzero(buffer, BUFSIZE);
  
  // read the request from the client  
  n = read(sock, buffer, BUFSIZE);

  // Print out the details of the client 
  printf("Received request from %s\n", inet_ntoa(cli_add.sin_addr));
  printf("Request: %s\n", buffer);

  if (n < 0) {
    printf("ERROR on reading from client socket\n");
    exit(1);
  }
  
  // Get the secretkey and file name from the request
  secretkey = strtok(buffer, "$");
  filename = strtok(NULL, "$");

  if (strcmp(secretkey, key) != 0){
    printf("ERROR: wrong key\n");
    exit(1);
  }
  else {

    // Using dup2 to redirect standard output
    //dup2(sock, 1);
   
    // Get the block size saved in the given file
    FILE *myfile = fopen(arg3, "r");
    fscanf(myfile, "%d", &blocksize);
    printf("Block size: %d\n", blocksize);

    // A buffer to read data
    char buf[blocksize];
    bzero(buf, blocksize);

    // Get the name of the directory in which we read data
    char file_path[100];
    bzero(file_path, 100);
    sprintf(file_path, "filedeposit/%s", filename);

    // Open the filename to read
    fd = open(file_path, O_RDONLY);
    if (fd == -1) {
      perror("ERROR on specified file");
      exit(1);
     }

    // Loops to read data, each time read blocksize bytes
    while (1) {
      bytes_read = read(fd, buf, blocksize);
      if (bytes_read < 0) {
        printf("ERROR on reading file\n");
        exit(1);
      }
      if (bytes_read == 0) {
        printf("End reading.\n");
        break;
      }

      // Send back to the client
      bytes_write = write(sock, buf, bytes_read);
      if (bytes_write != bytes_read){
        perror("ERROR: cannot write bytes read");
      }

      bzero(buf, blocksize);
    }

    // Close file descriptor
    close(fd);
  }  
   
  exit(0);
}
