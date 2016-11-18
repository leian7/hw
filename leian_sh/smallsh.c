/* Anne Lei
 * CS 344
 * Assign 3: smallsh
 * 11/17/2016
 */
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>

#define ARG_DELIM " \t\n"

static int curr_status = 0;

/* Grabs the line user enters as command prompt input */
char* grab_line() {
	char* line = NULL;
	size_t bufsize = 0;
	getline(&line, &bufsize, stdin);
	return line;
}


/* Cuts user's command line input argument by argument and returns resulting
 * array of strings */
char** cut_args(char* line) {
	int bufsize = 512;				// max num args
	int index = 0;					// current place in args array
	int i;							// for loop counter
	char* arg;						// each arg
	char** args;					// allocate array of args
	args = malloc(bufsize * sizeof(char*));

	arg = strtok(line, ARG_DELIM);
	while (arg != NULL) {
		args[index] = arg;
		index++;
		arg = strtok(NULL, ARG_DELIM);
	}
	args[index] = NULL;
	return args;
}


/* Checks to see if child processes have ended */
void check_children() {
	int status;
	pid_t spawnpid;

	while ((spawnpid = waitpid(-1, &status, WNOHANG)) > 0) {
		printf("bg process id was %d; ", spawnpid);
		fflush(stdout);

		if (WIFEXITED(status)) {
			printf("exited with status %d\n", status);
			fflush(stdout);
		}
		if (WIFSIGNALED(status)) {
			printf("terminated w signal %d\n", WTERMSIG(status));
			fflush(stdout);
		}
	}
}


/* Displays shell interface and accepts input */
void shell_loop() {
	char* line;						// user's command line input
	char** args;					// array for user's args	
	int status = 43;				// current status

	do {
		check_children();
		printf(": ");				// print the prompt
		fflush(stdout);
		line = grab_line();			// grab user's line
		args = cut_args(line);		// parse into args
		status = shell_execute(args);	// get status
		free(line);					// free mem
		free(args);

	} while (status != 0);
}


/* Forks the parent process, waits for child, and sets the shell status */
int shell_fork(char** args) {
	int count = 0;					// preliminary argument count
	int bg = 0;						// default: process not backgrounded

	char* input_redir = NULL;
   	char* output_redir = NULL;

	int input_file = STDIN_FILENO;
	int output_file = STDOUT_FILENO;

	struct sigaction action = {0};
	action.sa_handler = SIG_IGN;
	sigaction(SIGINT, &action, NULL);

	while (args[count] != NULL) {
		count++;
	}
	
	if (strcmp(args[count-1], "&") == 0) {
		bg = 1;						// user does want process to be bg
		args[count-1] = NULL;		// Now we know. Hack off &
	}

	if (args[1] != NULL) {
		if (strcmp(args[1], "<")==0) {
			if (args[2] == NULL) {
				printf("Error: Missing input file\n");
				fflush(stdout);
			}
			else {					// input file was provided
				input_redir = malloc(2048 * sizeof(char));
				strcpy(input_redir, args[2]);
			}
		}
		else if (strcmp(args[1], ">") == 0) {
			fflush(stdout);
			if (args[2] == NULL) {
				printf("Error: Missing output file\n");
				fflush(stdout);
			}
			else {
				output_redir = malloc(2048 * sizeof(char));
				strcpy(output_redir, args[2]);
			}
		}
	}
	pid_t spawnpid = -5;			// initialize to rand val
	int status = -5;				// initialize to rand val

	spawnpid = fork();				// fork process
	if (spawnpid == -1) {			// bad fork, set error
		perror("Fork unsuccessful; child process wasn't created\n");
		exit(1);					// bad status
	}
	else if (spawnpid == 0) {		// good fork: child case
		if (bg == 0) {
			action.sa_handler = SIG_DFL;
			action.sa_flags = 0;
			sigaction(SIGINT, &action, NULL);
		}
		if (input_redir != NULL) {
			input_file = open(input_redir, O_RDONLY);
			if (input_file == -1) {
				printf("Couldn't open %s for input\n", input_redir);
				fflush(stdout);
				exit(1);
			}
			else {
				args[1] = args[2] = NULL;
				dup2(input_file, STDIN_FILENO);
				close(input_file);
			}
		}
		else if (output_redir != NULL) {
			output_file = open(output_redir, O_WRONLY | O_CREAT, 0777);
			if (output_file == -1) {
				printf("Couldn't open output file\n");
				fflush(stdout);
			}
			else {
				args[1] = args[2] = NULL;
				dup2(output_file, STDOUT_FILENO);
				close(output_file);
			}
		}
		// if bad args sent to exec, set error
		if (execvp(args[0], args) == -1) {
			perror("Error: Couldn't find the command to run\n");
			curr_status = 1;
			exit(1);
		}
		// otherwise, we know child process present!! Don't exit
	}
	else {							// good fork: parent case
		if (input_redir != NULL) {
			free(input_redir);
		}
		if (output_redir != NULL) {
			free(output_redir);
		}

		if (bg == 0) {				// if not bg, wait	
			waitpid(spawnpid, &status, WUNTRACED);
			if (WIFEXITED(status)) {
				curr_status = WEXITSTATUS(status);
			}
			else if (WIFSIGNALED(status)) {
				curr_status = WTERMSIG(status);
				printf("terminated by signal %d\n", WTERMSIG(status));
			}
		}
		else {
			printf("bg process is %d\n", spawnpid);
		}
	}
	return 1;						// "everything ok" int indicator
}


/* Command for triggering the shell to exit; simply returns value of 0 to
 * indicate it's time to terminate for real */
int exit_command() {
	return 0;
}


/* Command for changing directories in the shell; returns value of 1 as 
 * status */
int cd_command(char** args) {
	if (args[1] == NULL) {
		chdir(getenv("HOME"));
	}
	else if (chdir(args[1]) != 0) {
		perror("Invalid destination\n");
	}
	return 1;
}


/* Retrieves recorded status value */
int status_command() {
	//based on scenario (signal vs. exit), print the curr status
	if (curr_status == 1 || curr_status == 0) {

		printf("exit value %d\n", curr_status);
	}
	else {
		printf("terminated by signal %d\n", curr_status);
	}
	fflush(stdout);
	return 1;
}


/* Executes user's requested command; either built-in (cd, exit, status) or
 * able to be handled by exec */
int shell_execute(char** args) {
	int i;
	if (args[0] == NULL || args[0][0] == '#') {
		return 1;					// ignore if empty or comment
	}
	if (strcmp(args[0], "exit") == 0) {
		return exit_command();		// 0 triggers exit
	}
	else if (strcmp(args[0], "cd") == 0) {
		return cd_command(args);	// 1 means run built-in
	} 
	else if (strcmp(args[0], "status") == 0) {
		return status_command();	//  1 means run built-in
	}
	else {
		return shell_fork(args);	// 1 means to let exec handle
	}
}


/* Main loops the shell interface constantly */
int main() {
	shell_loop();
	return 0;
}
