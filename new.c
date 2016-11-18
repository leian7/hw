#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

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

/* Displays shell interface and accepts input */
void shell_loop() {
	char* line;		// user's command line input
	char** args;	// array for user's args	
	int status = 43;		// current status

	do {
		printf(": ");				// print the prompt
		line = grab_line();			// grab user's line
		args = cut_args(line);		// parse into args
		status = shell_execute(args);	// get status

		free(line);					// free mem
		free(args);

	} while (status != 0);
}

/* Forks the parent process, waits for child, and sets the shell status */
int shell_fork(char** args) {
	pid_t spawnpid = -5;			// initialize to rand val
	int status = -5;				// initialize to rand val

	spawnpid = fork();				// fork process
	if (spawnpid == -1) {			// bad fork, set error
		perror("Fork unsuccessful; child process wasn't created\n");
		exit(1);					// bad status
	}
	else if (spawnpid == 0) {		// good fork: child case
		// if bad args sent to exec, set error
		if (execvp(args[0], args) == -1) {
			perror("Error: Couldn't find the command to run\n");
			curr_status = 1;
			exit(1);
		}
		// otherwise, we know child process present!! Don't exit
	}
	else {							// good fork: parent case
		// wait for our child as long as child has not terminated normally or
		// by signal
		do {
			waitpid(spawnpid, &status, 0);
			if (WIFEXITED(status)) {
				curr_status = WEXITSTATUS(status);
			}
			else if (WIFSIGNALED(status)) {
				curr_status = WSTOPSIG(status);
			}
			else if (WTERMSIG(status)) {
				curr_status = WTERMSIG(status);
			}
		} while (WIFEXITED(status) == 0 && WIFSIGNALED(status) == 0);
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
		fprintf(stderr, "cd requires argument\n");
	}
	else if (chdir(args[1]) != 0) {
		perror("Invalid destination\n");
	}
	return 1;
}


/* Retrieves recorded status value */
int status_command() {
	//call get_type to find out which scenario happend
	//based on scenario, print custom err msg + the curr status
	//}
	printf("%d\n", curr_status);
	fflush(stdout);
	return 1;
}

/* Executes user's requested command; either built-in (cd, exit, status) or
 * able to be handled by exe/c */
int shell_execute(char** args) {
	int i;
	if (args[0] == NULL || args[0][0] == '#') {
		return 1;					// ignore if empty or comment
	}
	if (strcmp(args[0], "exit") == 0) {
		return exit_command();		// 0 triggers exit
	}
	else if (strcmp(args[0], "cd") == 0) {
		return cd_command(args);		// 1 means run built-in
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
