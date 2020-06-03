#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>

bool coming_signal = false;
bool program_exit = false;

void sigint_handler(int signum){
	coming_signal = true;
}

void sigterm_handler(int signum){
	program_exit = true;
}


int Daemon(char** argv){
	signal(SIGINT, sigint_handler);
	signal(SIGTERM, sigterm_handler);

	char buf_exit[] = "I am daemn.I am ending\n";

	char buf[256];
	int fd_read = open(argv[1], O_RDWR, S_IRWXU);
	int count = read(fd_read,buf, sizeof(buf));
	close(fd_read);
	buf[count-1] = '\0';
	
	int fd_write = open("/tmp/output.txt", O_CREAT|O_RDWR, S_IRWXU);	
	close(1);
	dup2(fd_write, 1);

	lseek(fd, 0, SEEK_END); 
	
	while(!program_exit){
		if(coming_signal){
			pid_t parpid;
			if((parpid=fork()) == 0){       
		      		execve(buf, argv+1, NULL);         
			}
			coming_signal = false;
		}
	}	
	write(fd_write, buf_exit, sizeof(buf_exit));
	
	return 0;
}

int main(int argc, char* argv[])
{
	pid_t parpid;
	if((parpid=fork())<0) 
	{                
   		printf("\ncan't fork");
      		exit(1);           
	}
       	else if (parpid!=0)
        	exit(0);             
	printf("I am daemon.pid = %i\n", getpid());
	setsid();
	Daemon(argv);        
        return 0;
}



