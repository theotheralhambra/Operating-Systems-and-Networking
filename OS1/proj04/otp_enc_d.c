#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

//constants
char 	*CLIENT_ID = "enc!!";
const int BUF_SIZE = 1028;

// fn prototypes
void 	error(const char *msg);
void 	catchSIGCHLD(int signo);
void	encrypt(int plaintext_size, const char *txt, const char *key);

//globals
int		child_count = 0;
char 	*ciphertext;

int main(int argc, char *argv[]){																//argv[0] = program name, argv[1] = port

	int 			listenSocketFD, establishedConnectionFD, portNumber, charsRead;
	socklen_t sizeOfClientInfo;
	char 			buffer[BUF_SIZE];
	char			*plaintext, *key;//, *ciphertext;
	int				txt_size, key_size;
	struct 		sockaddr_in serverAddress, clientAddress;
	pid_t			pid, wpid;
	int				wstatus;
	struct		sigaction SIGCHLD_action = { 0 };
	
	if (argc < 2) { 																							// Check usage & args
		fprintf(stderr,"USAGE: %s port\n", argv[0]); 								// show correct usage
		exit(EXIT_FAILURE); 																				// die screaming
	}

	// signal handler setup
	SIGCHLD_action.sa_handler = catchSIGCHLD;			
	sigfillset(&SIGCHLD_action.sa_mask);
	SIGCHLD_action.sa_flags = SA_RESTART;													// might not need SA_RESTART
	sigaction(SIGCHLD, &SIGCHLD_action, NULL);										// no need to save defaults

	// Set up the address struct for this process (the server)
	memset((char *)&serverAddress, '\0', sizeof(serverAddress));	// Clear out the address struct
	portNumber = atoi(argv[1]);																		// Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET;														// Create a network-capable socket
	serverAddress.sin_port= htons(portNumber);										// Store the port number
	serverAddress.sin_addr.s_addr = htonl(INADDR_LOOPBACK);				// limit connections to localhost; (was INADDR_ANY)									
	
	// Set up the socket
	listenSocketFD = socket(AF_INET, SOCK_STREAM, 0);							// Create the socket
	if (listenSocketFD < 0) 
		error("OTP_ENC_D: ERROR opening socket");

	// Enable the socket to begin listening
	if (bind(listenSocketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)// Connect socket to port
		error("OTP_ENC_D: ERROR on binding");
	listen(listenSocketFD, 5);																		// Flip the socket on - it can now queue up 5 connections
	
	while(1) {
		// Accept a connection, blocking if one is not available until one connects
		sizeOfClientInfo = sizeof(clientAddress);										// Get the size of the address for the client that will connect
		if (child_count >= 5) {
			waitpid(-1, NULL, WUNTRACED);			
			//continue;			//else try this
		}
		establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *)&clientAddress, &sizeOfClientInfo);// Accept
	
		if (establishedConnectionFD < 0) {													// check that accept() worked - do not fork if it didn't
			error("OTP_ENC_D: ERROR on accept");
		} else {
			pid = fork();
			switch(pid) {
				case -1:																								// fork error
					error("OTP_ENC_D: ERROR on fork");												// we must warn the others
					break;
				case 0:																									// is child
					// get the first comm from client - this should be their ID
					memset(buffer, '\0', BUF_SIZE);												// clear out buffer
					charsRead = recv(establishedConnectionFD, buffer, BUF_SIZE - 1, 0);// Read the client's message from the socket	
					if (charsRead < 0) 																		
						error("OTP_ENC_D: ERROR reading from socket");
					
					// find out if we are talking to the right process - NOTE: this is not very secure
					if (strcmp(buffer, CLIENT_ID) != 0){									// incorrect process ID -> don't do crypto
						charsRead = send(establishedConnectionFD, "0", 1, 0);// tell client to get lost
						if (charsRead < 0) 
							error("OTP_ENC_D: ERROR writing to socket");
						close(establishedConnectionFD);											// Close the existing socket which is connected to the (wrong) client
						break;
					}

					// correct ID -> do crypto stuff
					charsRead = send(establishedConnectionFD, "1", 1, 0);	// tell client to proceed
					if (charsRead < 0) 												 
						error("OTP_ENC_D: ERROR writing to socket");

					/***** 1. get plaintext *****/
					// get size of plaintext
					memset(buffer, '\0', BUF_SIZE);												// clear out buffer
					charsRead = recv(establishedConnectionFD, buffer, BUF_SIZE - 1, 0);// Read the client's message from the socket	
					if (charsRead < 0) 																		
						error("OTP_ENC_D: ERROR reading from socket");
					// prepare to rec plaintext
					txt_size = atoi(buffer);															// convert the size to an int 
					plaintext = (char *)malloc(txt_size * sizeof(char));	// request enough memory to store the text
					memset(plaintext, '\0', txt_size);										// clear out the requested memory
					// tell client we are go for plaintext transmission
					charsRead = send(establishedConnectionFD, "1", 1, 0);	// tell client to proceed
					if (charsRead < 0) 												 
						error("OTP_ENC_D: ERROR writing to socket");
					// read plaintext
					while (strstr(plaintext, "!!") == NULL) {							// keep reading until we reach the end of the transmission					
						memset(buffer, '\0', BUF_SIZE);											// clear out buffer
						charsRead = recv(establishedConnectionFD, buffer, BUF_SIZE - 1, 0);// Read the client's message from the socket	
						strcat(plaintext, buffer);													// add rec'd text to complete text
						if (charsRead <= 0) { 															// make sure nothing went wrong						NOTE: might need to be just < not <=	
							error("OTP_ENC_D: ERROR reading from socket");
							break;
						}
					}	
					/***** END PLAINTEXT COMM *****/
					
					// signal client that we are ready for key size
					charsRead = send(establishedConnectionFD, "1", 1, 0);// tell client to proceed
					if (charsRead < 0) 												 
						error("ERROR writing to socket");


					/***** 2. get key *****/
					// get size of key
					memset(buffer, '\0', BUF_SIZE);												// clear out buffer
					charsRead = recv(establishedConnectionFD, buffer, BUF_SIZE - 1, 0);// Read the client's message from the socket	
					if (charsRead < 0) 																		
						error("OTP_ENC_D: ERROR reading from socket");
					// prepare to rec key
					key_size = atoi(buffer);
					key = (char *)malloc(key_size * sizeof(char));				// request enough memory to store the key
					memset(key, '\0', key_size);													// clear out the requested memory
					// tell client we are go for key transmission
					charsRead = send(establishedConnectionFD, "1", 1, 0);	// tell client to proceed
					if (charsRead < 0) 												 
						error("OTP_ENC_D: ERROR writing to socket");
					// read key
					while (strstr(key, "!!") == NULL) {										// keep reading until we reach the end of the transmission					
						memset(buffer, '\0', BUF_SIZE);											// clear out buffer
						charsRead = recv(establishedConnectionFD, buffer, BUF_SIZE - 1, 0);// Read the client's message from the socket	
						strcat(key, buffer);																// add rec'd text to complete text
						if (charsRead <= 0) { 															// make sure nothing went wrong						NOTE: might need to be just < not <=	
							error("OTP_ENC_D: ERROR reading from socket");
							break;
						}
					}	
					/***** END KEY COMM *****/

					// do crypto
					encrypt(txt_size, plaintext, key);

					// send back ciphertext
					charsRead = 0;
					while (charsRead < txt_size) {
						charsRead += send(establishedConnectionFD, ciphertext + charsRead, txt_size - charsRead, 0);	// tell client to proceed
						if (charsRead < 0) 												 
							error("OTP_ENC_D: ERROR writing to socket");
					}

					close(establishedConnectionFD);													// Close the existing socket which is connected to the client
					return 0;
				default:																									// is parent
					close(establishedConnectionFD);													// Close the parent's copy of existing socket - parent doesn't do the talking
					child_count += 1;																				// increment child count before continuing
			}
		}
	}

	close(listenSocketFD);																				// Close the listening socket

	return 0;
}

/*
 * simple function to spit out errors with specified prefixes
 * prereqs: none
 * returns: none
 */
void error(const char *msg) { 

	perror(msg); 
	exit(EXIT_FAILURE); 

}

/*
 * basic signal handler to ensure zomie child processes are reaped whenever they finish
 * prereqs: none
 * returns: none
 */
void catchSIGCHLD(int signo) {

	child_count -= 1;														// decrement the child count
	waitpid(-1, NULL, WNOHANG);									// reap the zombie child

}

/*
 * performs otp encryption on a char string
 * prereqs: ciphertext declared as char *
 * returns: none
 */
void encrypt(int plaintext_size, const char *txt, const char *key) {

	int p_char, k_char, c_char;
	int i;
	char c[1];

	ciphertext = (char *)malloc(plaintext_size * sizeof(char));										// make space for cipher - is equal to plaintext in size

	for (i = 0; i < plaintext_size - 4; i++) {																		// walk through each char (-4 to avoid transmission terminations and other junk chars)
		p_char = (int)txt[i] - 64;																									// normalize form ascii to base 27
		k_char = (int)key[i] - 64;																									//		A = 65 -64 = 1
		if (p_char == -32)																													// convert what was a space to 27th char
			p_char = 27;
		if (k_char == -32)
			k_char = 27;
		c_char = p_char + k_char;//(int)txt[i] + (int)key[i];//p_char + k_char			// add vals
		c_char %= 27;																																// modulo 27 (1-26 = A-Z + space)
		c_char += 64;																																// convert back to ascii
		ciphertext[i] = (char)c_char;																								// cast to char
	}					
	ciphertext[i] 	= '!';																												// tack transmission terminators onto the end
	ciphertext[i+1] = '!';
	ciphertext[i+2] = '\0';
}
