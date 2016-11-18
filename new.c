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
		printf("about to cut args\n");
		args = cut_args(line);		// parse into args
		printf("about t oget status\n");
		status = shell_execute(args);	// get status
		printf("about to free line\n");
		free(line);					// free mem
		printf("about to free args\n");
		free(args);

	} while (status != 0);
}

/* Forks the parent process, waits for child, and sets the shell status */
int shell_fork(char** args) {
//	char* input_redir = NULL;
   	char* output_redir = NULL;

//	int input_file = STDIN_FILENO;
	int output_file = STDOUT_FILENO;
	printf("done with dec\n");
/*	if (strcmp(args[1], "<")==0) {
		printf("yes there is a <\n");
		fflush(stdout);
		if (args[2] == NULL) {
			printf("Error: Missing input file\n");
			fflush(stdout);
		}
		else {						// input file was provided
			input_redir = malloc(2048 * sizeof(char));
			strcpy(input_redir, args[2]);
		}
	}
	else */ 
	if (args[1] != NULL) {
	if (strcmp(args[1], ">") == 0) {
		printf("Yes, there is a >\n");
		fflush(stdout);
		if (args[2] == NULL) {
			printf("Error: Missing output file\n");
			fflush(stdout);
		}
		else {
			output_redir = malloc(2048 * sizeof(char));
			strcpy(output_redir, args[2]);
			printf("we got here\n");
		}
	}}
	printf("dec pids\n");
	pid_t spawnpid = -5;			// initialize to rand val
	int status = -5;				// initialize to rand val

	printf("about to fork\n");
	spawnpid = fork();				// fork process
	if (spawnpid == -1) {			// bad fork, set error
		perror("Fork unsuccessful; child process wasn't created\n");
		exit(1);					// bad status
	}
	else if (spawnpid == 0) {		// good fork: child case
		printf("we're child\n");
		fflush(stdout);
/*		if (input_redir != NULL) {
			input_file = open(input_redir, O_RDONLY);
			if (input_file == -1) {
				printf("Couldn't open input file\n");
				fflush(stdout);
			}
			else {
				dup2(input_file, STDIN_FILENO);
				close(input_file);
			}
		} */
		if (output_redir != NULL) {
			printf("output redir isn't null\n");
			fflush(stdout);
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
	//	execvp(args[0], args);
	//	printf("didn't work?\n");
	//	fflush(stdout);
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
			if (output_redir != NULL) {
				free(output_redir);
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
		printf("about to fork args\n");
		return shell_fork(args);	// 1 means to let exec handle
	}
}

/* Main loops the shell interface constantly */
int main() {
	shell_loop();
	return 0;
}
