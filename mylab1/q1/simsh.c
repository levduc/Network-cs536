// simple shell example using fork() and execlp()

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

int main(void)
{
pid_t k;
char buf[100];
int status;
int len;
  while(1) {

	// print prompt
  	// print the process ID of the calling process
	fprintf(stdout,"[%d]$ ",getpid());
	// read command from stdin
	fgets(buf, 100, stdin);
	len = strlen(buf);
	if(len == 1) 				// only return key pressed
	  continue;
	buf[len-1] = '\0';
	//create a new separate process
  	k = fork();
	if (k==0) { //return 0 when called in the child process
  	// child code
	    if(execlp(buf,buf,NULL) == -1)	// if execution failed, terminate child
		{
			perror("execlp");
		   	exit(1);
		}   
	}
  	else {	
	// parent code 
	// terminate child process
     	waitpid(k, &status, 0);
  	}
  }
}
