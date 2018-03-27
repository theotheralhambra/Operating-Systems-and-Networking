/*
 * smallsh - a basic LINUX shell environment
 * Developed by Rick Menzel (menzelr@oregonstate.edu)
 *
 * To Do:
 * -background and foreground processes
 * -complete signal handling
 *  	-SIGINT
 *  	-SIGCHLD
 * -fix smallsh_exit()
 * -fix smallsh_status()
 * -fix $$ expansion so test$$ gets expanded too
 */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/types.h>

//constants
const char 		DELIMITERS[] = " \t\n\r\a";	//for strtok
const size_t 	INPUT_BUFFER_SIZE = 2048;		//per program specifications
const size_t	TOKEN_BUFFER_SIZE = 512;		//per program specifications
const int 		NUM_BUILTINS = 3;						//per program specifications + $$

// fn prototypes
void 	cmd_loop();
char	*read_line();
char	**parse_line();
int		execute();
int		launch();
int		redirect_IO();
void	shift_args();
void	SIGCHLD_msg();
int		smallsh_cd();			//builtin
int 	smallsh_exit();		//builtin
int		smallsh_status();	//builtin
void	catchSIGINT(int signo);		//sig handler
void	catchSIGTSTP(int signo);	//sig handler
void	catchSIGCHLD(int signo);	//sig handler


//globals
char *builtin_list[] = { "cd", "exit", "status" };
int  (*builtin_cmd[])(char **) = { 	&smallsh_cd, 
																		&smallsh_exit, 
																		&smallsh_status };
int		background_flag = 0;
int 	background_lock = 0;
int		integer_size = sizeof(int);
struct sigaction 	default_action = { 0 },
									ignore_action  = { 0 };
int		status;
int		sig;
int		last_child;
int		dead_child 	= 0;
int		is_child 		=	0;
int		child_count = 0;
int		child_list[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
											 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
											 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
											 0, 0 };

int main(){

	/********************* sig handling ********************/
	struct sigaction 	SIGTSTP_action = { 0 },			//initialize handlers
									 	SIGCHLD_action = { 0 };

	SIGTSTP_action.sa_handler = catchSIGTSTP;			//setup SIGTSTP
	sigfillset(&SIGTSTP_action.sa_mask);
	SIGTSTP_action.sa_flags		= SA_RESTART;				// !IMPORTANT! - see above

	SIGCHLD_action.sa_handler = catchSIGCHLD;			//setup SIGCHLD
	sigfillset(&SIGCHLD_action.sa_mask);
	SIGCHLD_action.sa_flags		= SA_RESTART;				// !IMPORTANT!  - see above

	ignore_action.sa_handler  = SIG_IGN;
	default_action.sa_handler = SIG_DFL;

	sigaction(SIGINT,  &ignore_action,  NULL);		//set sigactions
	sigaction(SIGTSTP, &SIGTSTP_action, NULL);
	sigaction(SIGCHLD, &SIGCHLD_action, NULL);
	/******************* end sig handling ******************/

	//do work
	cmd_loop();

	//cleanup?

	return EXIT_SUCCESS;

}

/*
 * basic shell ctrl loop function
 * prereqs: none
 * returns: none
 */
void cmd_loop(){

	char 	*buffer;								//read buffer
	char 	**args;									//parsed cmd list
	int		i;									

	do{

		//background_flag = 0;				//reset background flag
		//sig						  = 0;				//reset signal

		write(1, ": ", 2);					//show prompt per specifications
		fflush(stdout);							//flush output buffers
		//if (dead_child)
		//	SIGCHLD_msg(1);

		buffer = read_line();				//read in user input
		args   = parse_line(buffer);//parse input into arguments
		status = execute(args);			//execute the parsed cmd string
		//if (dead_child)
		//	SIGCHLD_msg(1);
		free(buffer);								//clean up read buffer
		free(args); 								//clean up parsed cmd list
		//dead_child = 0;
	} while (status != 3);

}

/*
 * simple function to read in a line from stdin
 * prereqs: none
 * returns: a line of input (as char *)
 */
char *read_line(){

	ssize_t 	size = 0;//, e, len, ret;				
	char 			*buffer = NULL;

	getline(&buffer, &size, stdin);
 	//while (len != 0 && (ret = read(STDIN_FILENO, buffer, len)) != 0) {
	//	if (ret == -1) {
	//		if (errno == EINTR) 
	//			continue;
	//		break;
	//	}
	//}

	return buffer;

}

/*
 * function to split (parse) a line of input into a list of individual arguments
 * performs var expansion (currently only $$) and detects background requests 
 * prereqs: input read into buffer
 * returns: a list of argument tokens (as char **)
 */
char **parse_line(char *buffer){

	size_t 	size = TOKEN_BUFFER_SIZE;
	char		**args = (char **)malloc(size * sizeof(char));
	char		*str, *strptr, temp[20], *ret;
	int 		i;

	if (!args) {											//arg list was not properly initialized
		fprintf(stderr, "parse_line(): args allocation error!\n");
		fflush(stderr);
		exit(EXIT_FAILURE);
	}

	for (i = 0, str = buffer; ; i++, str = NULL) {	//work through the buffer
		args[i] = strtok_r(str, DELIMITERS, &strptr);	//get the next token
		if (args[i] == NULL) {												//NULL indicates end of buffer
			if (i > 0) {
				background_flag = 0; 												
				if ( strcmp(args[i-1], "&") == 0 ) {			//user requested background	
					args[i-1] = NULL; 											//get rid of the &
					if (!background_lock) { background_flag = 1; }//set background flag
				}																					//if not locked out
			}
			return args;																//arg list finished->return
		}
		ret = strstr(args[i], "$$");									//$$ expansion
		if (ret != NULL) {														//search str
			strncpy(ret, "%d", 2);											//replace $$ with %d
			//printf("hit, %s\n", args[i]); 
			sprintf(temp, args[i], getpid());						//replace %d with pid
			args[i] = temp;
		}
		//if ( strcmp(args[i], "$$") == 0 ){						//PID var -> expand
		//	sprintf(temp, "%d", getpid());							//get PID
		//	args[i] = temp;															//replace arg with PID
		//}																//cannot use strcpy/etc on args[i]!!
	}																	//reassign pointer to valid str instead.

}

/*
 * funtion to execute command in the argument list
 * prereqs: args[] initialized
 * returns: return status of executed cmd (0 = success, 1 = failure, 3 = exit)
 */
int execute(char **args){

	int i;

	if (args[0] == NULL || args[0][0] == '#')			//blank line or comment entered
		return 0;

	for (i = 0; i < NUM_BUILTINS; i++) {					//check if cmd is a 'built-in'
		if ( strcmp(args[0], builtin_list[i]) == 0 )
			return (*builtin_cmd[i])(args);						//if so, execute built-in
	}

	return launch(args);													//else perform fork/exec

}

/*
 * 'built-in' function to allow smallsh change cwd
 * prereqs: args[] populated
 * returns: 1 on success, 2 on non-success
 */
int smallsh_cd(char **args){

	if (args[1] == NULL) {						//no target dir specified -> go home	
		if ( chdir(getenv("HOME")) != 0 ) {
			perror("smallsh:smallsh_cd()");
			return 1;
		}
		return 0;
	} else {													//target dir argument present
		if ( chdir(args[1]) != 0 ) {		//cd failed - missing dir or bad permissions
			perror("smallsh:smallsh_cd()");
			return 1;
		}
	}

	return 0;													//success

}

/*
 * 'built-in' function to allow smallsh to perform a controlled exit
 * prereqs: args[] populated (not used)
 * returns: 0 to initiate exit from parent do..while loop
 */
int smallsh_exit(char **args){

	int i;
	//kill all the children
	for (i = 0; i < child_count; i++)// in child_list..kill em all
		kill(child_list[i], SIGQUIT);
	return 3;

}

/*
 * 'built-in' function to print last return status to stdout
 * prereqs: args[] populated (not used)
 * returns: 1 on success, 2 on non-success (not used)
 */
int smallsh_status(char **args){

	//get the last status or signal
	if (sig) {																		//is sig is set, last process killed by signal	
		printf("terminated by signal %d\n", sig);
		fflush(stdout);
	} else {																			//otherwise it was a normal exit
		printf("exit value %d\n", status);
		fflush(stdout);
	}
	return 0;	

}

/*
 * external program launcher, forks process and execs into external program
 * prereqs: args populated
 * returns: 1 on success, 2 on non-success
 */
int launch(char **args){

	pid_t pid, wpid;
	int wstatus, i;

	pid = fork();
	switch(pid) {

		case -1:															//fork error
			fprintf(stderr, "fork() error");		//print error
			fflush(stderr);
			return 1;														//die
		case 0:																//is child
			is_child = 1;
			if (!background_flag) 
				sigaction(SIGINT,  &default_action, NULL);//reset sigaction
			child_count = 0;										//has no children yet, reset count...
			for (i = 0; i < 32; i++)						//...and reset list
				child_list[i] = 0;				
			if (redirect_IO(args))								//redirect i/o (exit child on error)
				exit(1);
			if (execvp(args[0], args) == -1) {	//execution error
				printf("%s: no such file or directory\n", args[0]);
				fflush(stdout);
			}
			exit(1);														//die - should never be here
		default:															//is parent
			child_list[child_count] = pid;			//add child pid to list
			child_count++;											//add child to tally

			if (background_flag) {							//child running in background
				printf("background pid is %d\n", pid);
				fflush(stdout);
				return 0;
			} else {														//child in foreground
				do {															//wait for child to exit
					wpid = waitpid(pid, &wstatus, WUNTRACED);
				} while ( !WIFEXITED(wstatus) && !WIFSIGNALED(wstatus) );
				last_child = wpid;

				if ( WIFEXITED(wstatus) ) {
					sig = 0;
				} else if ( WIFSIGNALED(wstatus) ) {
					sig = WTERMSIG(wstatus);
				}
				SIGCHLD_msg(0);
			}
	}

	return wstatus == 256 ? 1 : wstatus;		//only parent will get here

}

/*
 * function to detect I/O redirection and set I/O using dup2() as specified
 * prereqs: args populated
 * returns: 1 on success, 2 on non-success - mods args if redirects are present
 */
int redirect_IO(char **args){

	int i, size = TOKEN_BUFFER_SIZE;
	int sourceFD, targetFD, result;

	for (i = 0; i < size; i++) {												//walk through args list	
		if (args[i] == NULL) {														//end of args
			return 0;
		} 

		if ( args[i][0] == '<' ) {												//source redirect found
			sourceFD = open(args[i+1], O_RDONLY);						//open source file
			if (sourceFD == -1) {														//open() error
				printf("cannot open %s for input\n", args[i+1]);
				fflush(stdout);
				return 1;																			//exit child on error
			}												
			shift_args(args, i);														//get rid of redir tokens
			result = dup2(sourceFD, 0);											//set source
			if (result == -1) {															//dup2() error
				printf("cannot set %s as source\n", args[i+1]);
				fflush(stdout);
				return 1;																			//exit child on error
			}
			--i;	//decrement i - shift_args() will put different args at args[i]
			continue;																				//IMPORTANT
		} 
		
		if ( args[i][0] == '>' ) {												//target redirect
			targetFD = open(args[i+1], O_WRONLY | O_CREAT | O_TRUNC, 0644);//open file
			if (targetFD == -1) {														//open() error
				printf("cannot open %s for output\n", args[i+1]);
				fflush(stdout);
				return 1;																			//exit child on error
			}
			shift_args(args, i);														//get rid of redir tokens
			result = dup2(targetFD, 1);											//set target
			if (result == -1) {															//dup2() error
				printf("cannot set %s as source\n", args[i+1]);
				fflush(stdout);
				return 1;																			//exit child on error
			}
			--i; //decrement i - shift_args() will put different args at args[i]
			continue;																				//IMPORTANT
		}
	}//end for loop

}

/*
 * helper function used to throw out redirect tokens from args if needed
 * prereqs: args populated
 * returns: none - changes args
 */
void shift_args(char **args, int index){

	int i, size = TOKEN_BUFFER_SIZE - 2;

	for (i = index; i < size; i++) { //i += 2
		args[i] = args[i+2];						//replace < or > operator
		args[i+1] = args[i+3];					//replace file argument

		if (args[i] == NULL)						//end of args
			return;
	}

}

/*
 * NOT USED
 * n/a
 * n/a
 */
void catchSIGINT(int signo){

	return;

}

/*
 * signal handler to allow SIGTStP to toggle background mode on/off
 * prereqs: none
 * returns: none
 */
void catchSIGTSTP(int signo){

	if (background_lock) {															//already locked..
		write(1, "\nExiting foreground-only mode\n", 30);
		fflush(stdout);
		background_lock = 0;															//open up
	} else {																						//otherwise lock
		write(1, "\nEntering foreground-only mode (& is now ignored)\n", 50);
		fflush(stdout);
		background_lock = 1;
	}

	write(1, ": ", 2);				//reprint prompt
	fflush(stdout);

}

/*
 * signal handler for sigchld
 * prereqs: none
 * returns: none
 */
void catchSIGCHLD(int signo){

	dead_child = 1;																//set flag

	int cpid;
	int cmethod, cstatus, csignal;

	//write(1, "\nin child handler\n", 18);
	//fflush(stdout);
	cpid = waitpid(-1, &cmethod, WNOHANG);				//get the pid

	if (cpid == -1) {															//already reaped
		//write(1, "\nerror in catchSIGCHLD\n", 23);
 		return;
	}
	last_child = cpid;

	if ( WIFEXITED(cmethod) ) {										//figure out status junk
		cstatus = WEXITSTATUS(cmethod);							//and place in globals
		status  = cstatus;
		sig 		= 0;  
	} else if ( WIFSIGNALED(cmethod) ) {
		csignal = WTERMSIG(cmethod);
		sig 		= csignal; 	
	}
	//write(1, "exit value 1\n", 13);
	SIGCHLD_msg(1);
}

/*
 * prints out termination junk	
 * prereqs: flags set
 * returns: none
 */
void SIGCHLD_msg (int prompt) {
	
	if (background_flag) {																	//background process
		printf("\nbackground pid %d is done: ", last_child);
		fflush(stdout);
		if (sig) {																						//signaled
			printf("terminated by signal %d\n", sig);
			fflush(stdout);
		} else {																							//exited normally
			printf("exit value %d\n", status);
			fflush(stdout);
		}
		if (prompt) {
			write(1, ": ", 2);																		//print prompt
			fflush(stdout);
		}
	} else if (sig) {																				//signaled foreground
		printf("process %d terminated by signal %d\n", last_child, sig);
		fflush(stdout);
		if (prompt) {
			write(1, ": ", 2);																		//print prompt
			fflush(stdout);
		}	
	} //else {
	//	printf("process %d terminated by signal %d\n", last_child, status);
	//	fflush(stdout);
	//	if (prompt) {
	//		write(1, ": ", 2);																		//print prompt
	//		fflush(stdout);
	//	}	
	//}

	dead_child = 0;																					//reset trigger

}


