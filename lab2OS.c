#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <pwd.h>
#include <linux/limits.h>

/* GLOBAL VARIABLES */
volatile sig_atomic_t is_child = 0;
pid_t child_pid[] = {};
int num_child = 0;
/* FUNCTIONS's DECLARATION */
void Title();
int cd_cmd(const char* path);

char** ReadInput();
void SpawnProcess(char** args);

void PauseChildProcess(pid_t pid);
void ContinueChildProcess(pid_t pid);
void Ctrl_C(int sig);

int main()
{
	system("clear");
	signal(SIGINT, Ctrl_C);
	// get user name
	register struct passwd *pw;
  	register uid_t uid;
  	uid = geteuid ();
  	pw = getpwuid (uid);
  	
  	// store name of current working directory
  	char cwd[100];
  	
  	// start the terminal
	Title();

	while(1)
	{
		// showing get-input line like normal shell shows
		// just for asthetic
		if(getcwd(cwd, 100) != NULL && pw != NULL)
		{
			printf("\n%s@%s: ",pw->pw_name, cwd);
		}
		
		// read input (command)
		char** args = ReadInput();
		
		
		// if user doesn't enter anything
		// then do nothing
		if(NULL == args[0]) {continue;}
		
		// if spot close command
		// then end the program
		else if(strcmp(args[0], "close") == 0) {break;}
		
		// if spot cd command, then apply builtin cd function
		// because normal cd command is a builtin with shell
		// and using exec() to call cd will only affects in the process
		// calling it, not the parent process
		// so we will see nothing happen on the screen
		else if(strcmp(args[0], "cd") == 0) {cd_cmd(args[1]);}
		
		// execute builtin pause command
		else if(strcmp(args[0], "pause") == 0)
		{
			int pid = atoi(args[1]);
			PauseChildProcess(pid);
		}
		
		// execute builtin continue command
		else if(strcmp(args[0], "continue") == 0)
		{
			int pid = atoi(args[1]);
			ContinueChildProcess(pid);
		}
		
		// otherwise execute the command
		else {SpawnProcess(args);}
		
		// delete previous input
		free(args);
	}
	return 0;
}

int cd_cmd(const char* path)
{
	if(chdir(path) != 0)
	{
		printf("Failed to change diretory\n");
		if(errno == ENOENT)
		{
			perror(path);
			return 1;
		}
	}	
	return 0;
}

char** ReadInput()
{
	// elements's postion in input array "tokens"
	int pos = 0;
	
	// create buffer to read input
	char* buffer = NULL;
	
	// size of buffer
	ssize_t buffer_size = 0;
	
	// read input
	getline(&buffer, &buffer_size, stdin); 
	char** tokens = malloc(10 * sizeof(char*));
	char* token;
	char delimiters[]=" \t\r\n\v\f";
	token = strtok(buffer, delimiters);
	
	while (token != NULL)
	{
		tokens[pos] = token;
		pos++;
		token = strtok(NULL, delimiters);
	}
	tokens[pos] = 0;
	return tokens;
}

void SpawnProcess(char** args)
{
	pid_t pid;
	pid_t childpid;
	int child_stat;
	pid = fork();
	switch(pid)
	{	
		case -1:
		{
			printf("fork() failed\n");
			return;
			break;
		}	
		case 0:
		{
			printf("\n");
			execvp(args[0], args);
			printf("execvp() failed\n");
			exit(EXIT_FAILURE);
			break;
		}
		default:
		{
			child_pid[0] = pid;
			num_child++;
			//child_pid = waitpid(-1, &child_stat, WNOHANG);
			childpid = wait(&child_stat);
			break;
		}
	}
	
}
void PauseChildProcess(pid_t pid)
{
	if(kill(pid, SIGSTOP) != 0)
	{
		if(errno == ESRCH)
		{
			printf("Process with PID %d doesn't exits\n", pid);
		}
		printf("Sending stop signal failed\n");
	}
}
void ContinueChildProcess(pid_t pid)
{
	if(kill(pid, SIGCONT) != 0)
	{
		if(errno == ESRCH)
		{
			printf("Process with PID %d doesn't exits\n", pid);
		}
		printf("Sending continue signal failed\n");
	}
}

void Ctrl_C(int sig)
{
	//signal(sig, Ctrl_C);
	for(int i  =0; i < num_child; i++)
	{
		printf("%d\n", child_pid[i]);
		kill(child_pid[i], SIGINT);
	}
}
void Title()
{
	printf("\t\t\t\tWelcome to HUNG's terminal\n");
}
