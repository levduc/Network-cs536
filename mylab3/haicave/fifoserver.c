// simple shell example using fork() and execlp()

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#define BUFSIZE 100

// token the request to get pid and an array of arguments 
char* tokenzied(char * a[], char *str){
  char * delim = "$ ";
  int i = 0;
  char *ptr = strtok(str, delim);
  char *pid = ptr; 
  ptr = strtok(NULL, delim);
  while (ptr != NULL){
	a[i] = ptr;
	//printf("%s\n", a[i]); 
    ptr = strtok(NULL, delim);
    i++;
  }
  // Add null to the end 
  a[i] = NULL;
  return pid; 
}

int main(void)
{
  pid_t k;
  int status;
  char buf[100]="";
  char *cmdfifo = "cmdfifo";
  int client_fd, server_fd;
  int jobs [BUFSIZE] = {0};
  int count = 0; 
  
  printf("Sever is on\n");
  // create a file that a client process writes to
  mkfifo(cmdfifo, 0666);

  // open the FIFO
  server_fd = open(cmdfifo, O_RDONLY);
  if (server_fd == -1) {
    printf("ERROR : Can't open server FIFO\n");
  }

  while(1) {
    read(server_fd, buf, BUFSIZE);
    
    if(strcmp ("", buf) != 0){	 
      char *arg[10];
	  char * client_pid = tokenzied(arg, buf);	
      buf[0] = '\0';

      // the client's file name
      char c_filename[100] = "";
      strcat(c_filename, "cfifo");
      strcat(c_filename, client_pid);
      
      k = fork(); // Create a child process 
      if (k==0) { 
		// child code
     	
 		 printf("Created [%d] to execute the request %s from %s\n", 
				getpid(), arg[0], client_pid); 
		// Open client file to write 
		client_fd = open(c_filename, O_WRONLY);
	
		// Using dup2 to redirect standard output to client's fifo
		dup2(client_fd, 1);
				
		// replaces the current process image with a new process image
		execvp(arg[0], arg); 	// if execution failed, terminate child
		printf("Done in child %d\n", getpid());
		exit(1);
      }
      else {
		// parent code
	        jobs[count] = k;
		count++ ;
	      }
	    }
	
        // perform waitpid with option WNOHANG to monitor the status of 
	    // child processes. If some child has exited, removing it from 
	    // the list and print out information 
	    int i;
	    for (i = 0; i < count; i++){
	      if (jobs[i] > 0){
			int ret = waitpid(jobs[i], &status, WNOHANG);
			if (ret > 0){
		 	 jobs[i] = 0;
		  	printf("Child process %d terminated\n", ret);
		}	
	  }
	}
  }
}
	
