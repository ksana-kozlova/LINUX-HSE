#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <semaphore.h>

bool coming_signal = false;
bool program_exit = false;
bool child_signal = false;

sem_t sem;

void sigint_handler(int signum) {
	coming_signal = true;
}

void sigterm_handler(int signum) {
	program_exit = true;
}

void sigchld_handler(int signum) {
	child_signal = true;
}


void writeLogMsg(char *message)
{
	int fd = open("log.txt", O_CREAT | O_RDWR | O_APPEND, S_IRWXU);
	char fullMsg[512];
	char *autor = "DAEMON: ";

	strcpy(fullMsg, autor);
	strcat(fullMsg, message);
	strcat(fullMsg, "\n");

	write(fd, fullMsg, strlen(fullMsg));

	close(fd);
}

int commandExecuter(char **args)
{
	int output = open("output.txt", O_CREAT | O_RDWR | O_APPEND, S_IRWXU);

	dup2(output, 1);
	execv(args[0], args);

}


int Daemon(char** argv) {
	signal(SIGINT, sigint_handler);
	signal(SIGTERM, sigterm_handler);
	signal(SIGCHLD, sigchld_handler);

	if (sem_init(&sem, 0, 1) == -1)
	{
		writeLogMsg("Error, can't to initialize the semaphore");
		program_exit = true;
		writeLogMsg("Daemon ended work because of error");
	}

	char buf[4096];
	int fd_read = open(argv[1], O_RDWR, S_IRWXU);
	int count = read(fd_read, buf, sizeof(buf));
	close(fd_read);
	writeLogMsg("Start!");
	while (!program_exit) {
		pause();
		if (coming_signal) {
			writeLogMsg("Signal has comed");
			if (count > 0) {
				char *commands[64];
				int cnt = 0;
				char *tmp;
				tmp = strtok(buf, "\n");
				while (tmp != NULL) {
					commands[cnt++] = tmp;
					tmp = strtok(NULL, "\n");
				}

				for (int i = 0; i < cnt; i++) {
					pid_t pid;
					if ((pid = fork()) == 0) {

						char *args[64];
						int cntArg = 0;
						char *tmpArg;

						tmpArg = strtok(commands[i], " ");
						args[cntArg++] = tmpArg;

						while (tmpArg != NULL) {
							tmpArg = strtok(NULL, " ");
							args[cntArg++] = tmpArg;

						}

						args[cntArg] = NULL;

						if (sem_wait(&sem) == -1) {
							writeLogMsg("Error while sem_wait operation!");
						}
						else {
							char msg[100] = "Command is executed:";
							strcat(msg, args[0]);
							writeLogMsg(msg);
							commandExecuter(args);

						}

					}
					else if (pid > 0) {
						while (1) {
							if (child_signal) {
								waitpid(-1, NULL, 0);
								sem_post(&sem);
								child_signal = false;
								break;
							}
							pause();
						}
					}
				}
			}
			else {
				writeLogMsg("count == 0, error read file");
			}

			coming_signal = false;
		}

	}

	sem_destroy(&sem);
	writeLogMsg("Finish!");


	return 0;
}



int main(int argc, char* argv[])
{

	pid_t parpid;
	if ((parpid = fork()) < 0) {
		printf("\ncan't fork");
		exit(1);
	}
	else if (parpid != 0)
		exit(0);
	printf("I am daemon.pid = %i\n", getpid());
	setsid();
	close(1);
	Daemon(argv);
	return 0;
}


