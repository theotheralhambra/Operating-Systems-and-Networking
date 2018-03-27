/*
 * chatclient.c
 * programmed by: Rick Menzel
 * for: CS 372, Winter 2018
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <dirent.h>

// constants
const int 	COMM_BUF_SIZE = 256;
const int 	FILE_BUF_SIZE = 100000000;		// per instructor this is 100 MB = 100,000,000 Bytes; sizeof(char) = 1 Byte
const int 	INIT_MSG_SIZE = 7;						// corresponds to INIT_MSG & ACK_MSG lengths
const char 	INIT_MSG[] = "PORTNUM";
const char 	ACK_MSG[] = "ACKPROC";
//const char 	CMD_LIST[][] = {"-l", "-g"}; // easily extensible for added commands
//const int		CMD_COUNT = 2;

/*
 * simple helper to print errors to the terminal
 * prereqs: none
 * returns: none
 */
void error(const char *msg) { 

	perror(msg); 
	exit(0); 

}

/*
 * simple helper to print usage intructions to the terminal
 * prereqs: none
 * returns: none
 */
void usage(char *name) {
	fprintf(stderr,"USAGE: %s port\n", name);
}

/*
 * simple functio to pull leftover junk (newline) out of stdin after scanf(). Req due to fflush() behaviour being undefined on non-stdout
 * prereqs: none
 * returns: none. alters/clears stdin.
 */
void clean_stdin() {
	char c;
	do {																													// pull all the junk out of stdin until we reach \n or EOF
		c = getchar();
	} while(c != '\n' && c != EOF);

}

/*
 * simple helper to read in the user's handle from the terminal
 * prereqs: memory allocated for input string
 * returns: none. alters data stored in handle
 */
/*
void get_handle(char *handle) {

	printf("Enter handle (10 chars max, no spaces): ");
	scanf("%s", handle);	
	clean_stdin();
	printf("Welcome %s. Type \\quit to quit chatting.\n\n", handle);	

}
*/
/*
 * simple helper to print a prompt for user input message and read input to buffer
 * prereqs: memory allocated for buffer, handle; handle populated with value
 * returns: none. alters data stored in buffer
 */
/*
void prompt(char *handle, char *buffer) {

	printf("%s> ", handle);																				// display handle as prompt
	fgets(buffer, INPUT_BUF_SIZE, stdin);													// get msg from user

}
*/
/*
 * function to perform setup of TCP socket
 * prereqs: port number is valid
 * returns: file descriptor of established socket (int)
 */
int setup_socket(int portNumber) {

	int 		socketFD;
	struct 	sockaddr_in serverAddress;
	struct 	hostent* serverHostInfo;
	// set up the server address struct
	memset((char*)&serverAddress, '\0', sizeof(serverAddress));		// Clear out the address struct
	serverAddress.sin_family = AF_INET; 													// Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); 									// Store the port number
	serverAddress.sin_addr.s_addr = htonl(INADDR_ANY); 						// accept global connections (use INADDR_LOOPBACK to limit to localhost)
	
	// set up the socket
	socketFD = socket(AF_INET, SOCK_STREAM, 0);										// Create the socket
	if (socketFD < 0) {																						// verify that socket creation did not produce an error
		error("ftserver: ERROR opening socket. Exiting...");
		exit(1);
	}

	// enable socket to begin listening
	if (bind(socketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {	// connect socket to port
		error("ftserver: ERROR on bind. Exiting...");								// verify that binding did not produce an error
		exit(1);
	}

	return socketFD;

}

/*
 * performs initial connection request and recieves acknowledgement
 * prereqs: socketFD is a valid socket
 * returns: none
 */
/*
void setup_connection(int socketFD) {

	char buffer[INIT_MSG_SIZE];

	if ( send(socketFD, INIT_MSG, INIT_MSG_SIZE, 0) < 0 ) {			// send initiation request to server; < 0 indicates send error
		error("chatclient: ERROR writing to socket. Could not establish connection. Exiting...");
		exit(1);
	}
	memset(buffer, '\0', INIT_MSG_SIZE);												// Clear out the buffer
	if ( recv(socketFD, buffer, INIT_MSG_SIZE, 0) < 0) {				// Read data (init ack) from the server, leaving \0 at end; < 0 indicates rec error
		error("chatclient: ERROR reading from socket. Could not establish connection. Exiting...");
		exit(1);
	}	
	if( strcmp(buffer, ACK_MSG) != 0 ) {												// verify that server ack's init request; 0 indicates match
		error("chatclient: ERROR establishing connection. Server did not acknowledge init request. Exiting...");
		exit(1);
	}

}
*/

/*
 *
 *
 *
 */
int lookup_command(char *buffer) {
	
	if ( strstr( buffer, "-l") ) {				// list
		return 0;
	} else if ( strstr(buffer, "-g") ) {	// get
		return 1;
	} else {															// not found
		return -1;
	}
	
}


void invalid_command(int connectionFD, char *buffer) {

	memset(buffer, '\0', sizeof(buffer));														// clear out the buffer. NOTE: uses sizeof for reusability
	strcat(buffer, "ERROR invalid command");
	if (send(connectionFD, buffer, sizeof(buffer), 0) < 0)
		error("ftserver: ERROR writing invalid_command() to socket.");

}


void invalid_file(int connectionFD, char *buffer) {
	
	memset(buffer, '\0', sizeof(buffer));														// clear out the buffer. NOTE: uses sizeof for reusability
	strcat(buffer, "ERROR no such file");
	if (send(connectionFD, buffer, sizeof(buffer), 0) < 0)
		error("ftserver: ERROR writing invalid_file() to socket.");
}

void list_files(int connectionFD, char *buffer) {

	DIR			*dptr = opendir(".");																		// open current directory
	struct	dirent *fptr = NULL;																		// set file ptr to null
	int			charsWritten = 0, i;
	//struct 	stat dStat;																							// initialize stat data container -- not needed
	//char		fName[30];																							// container for file names				-- not needed

	memset(buffer, '\0', sizeof(buffer));														// clear out send buffer
	while ((fptr = readdir(dptr)) != NULL) {												// while there are more files in dir
		if (strcmp(fptr->d_name, ".") != 0 && strcmp(fptr->d_name, "..") != 0) // don't list . and .. 
			strcat(buffer, fptr->d_name);																// add the file name to the send buffer
			// add delimiting char??
	}
	// send
	while (charsWritten < sizeof(buffer)) {
		i = send(connectionFD, buffer + charsWritten, sizeof(buffer) - charsWritten, 0);
		if (i < 0)
			error("ftserver: ERROR writing to socket");	
			// break??
		charsWritten += i;
	}
	// send
} 

int valid_file(char *target) {
	DIR			*dptr = opendir(".");																		// open current directory
	struct	dirent *fptr = NULL;																		// set file ptr to null
	int			charsWritten = 0, i;

	while ((fptr = readdir(dptr)) != NULL) {												// while there are more files in dir
		if (strcmp(fptr->d_name, "..") == 0) // don't list . and .. 
			return 1;
	}
	return 0;

}

/*
 * simple helper function to pass along quit request to server
 * prereqs: buffer == "\quit", socketFD is an open connection
 * returns: none
 */
/*
void send_close(int socketFD, char *buffer) {

	if( send( socketFD, buffer, INPUT_BUF_SIZE, 0 ) < 0 )						// send quit message to server
			error( "chatclient: ERROR writing to socket" );	

}
*/
int main( int argc, char *argv[] ){ 															// argv[0] = exe, argv[1] = hostname, argv[2] = port

	int 			socketFD, controlConnectionFD, dataConnectionFD, charsWritten, charsRead;
	socklen_t	sizeOfClientInfo;
	struct 		sockaddr_in clientAddress;
	//char			comm_buffer[COMM_BUF_SIZE];
	//char			file_buffer[FILE_BUF_SIZE];
	char			*comm_buffer;
	char			*file_buffer;	
	
	comm_buffer = (char *)malloc(sizeof(char) * COMM_BUF_SIZE);
	file_buffer = (char *)malloc(sizeof(char) * FILE_BUF_SIZE);	


	if ( argc < 2 ) { 																							// program, port
		usage( argv[0] );
		exit(0); 
	}

	socketFD = setup_socket( atoi(argv[1]) );												// setup TCP socket
	listen(socketFD, 5);																						// listen on that socket; baclog size = 5
	//setup_connection( socketFD );																		// setup connection with server
	printf("ftserver: listening on port %d\n", atoi(argv[1]));				// verbose
	/*
	while( 1 ) {
		//Accept a connection, blocking until one connects if not immediately available			
		sizeOfClientInfo = sizeof(clientAddress);
		controlConnectionFD = accept(socketFD, (struct sockaddr *)&clientAddress, &sizeOfClientInfo); // accept connection
		if (controlConnectionFD < 0)																	// verify that accept did not produce an error
			error("ftserver: ERROR on accept.");
		//else fork() ?? --  not needed
		//setup control connection?
		
		do {
			// need to wrap in a loop
			memset(comm_buffer, '\0', COMM_BUF_SIZE);
			charsRead = recv(controlConnectionFD, comm_buffer, COMM_BUF_SIZE - 1, 0); // read the client's message from socket
			if (charsRead < 0) 																						// verify that read did not produce an error
				error("ftserver: ERROR reading from socket");
		
			//check recvd command
			switch (lookup_command(comm_buffer)) {
				case -1:		// invalid command
					invalid_command(controlConnectionFD, comm_buffer);
				case 0: 		//list
					list_files(controlConnectionFD, file_buffer);
				case 1: 		//get
					if (valid_file(comm_buffer + 3)) {
						continue;
						// send_file(controlConnectionFD, comm_buffer);
					} else {
						invalid_file(controlConnectionFD, comm_buffer);
					}
			}
		} while(1);

		close(controlConnectionFD);																		// close the server side of the controlconnection
		
		//memset( comm_buffer, '\0', COMM_BUF_SIZE );										// Clear out the comm buffer

		//if( recv( socketFD, comm_buffer, COMM_BUF_SIZE - 1, 0 ) < 0 ) // Read data from the socket, leaving \0 at end
		//	error("chatclient: ERROR reading from socket");
		//if( strcmp( comm_buffer, "\\quit" ) == 0 ) {									// check for quit command from server
		//	printf("Connection terminated by server. Exiting...\n");
		//	break;
		//}
		
	}
	*/
	free(comm_buffer);
	free(file_buffer);
	close(socketFD);																								// close the listening socket before exit
	return 0;																												

}
