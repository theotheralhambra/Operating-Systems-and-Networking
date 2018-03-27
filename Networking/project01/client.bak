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

// constants
const int INPUT_BUF_SIZE = 500;					// per program reqs
const int HANDLE_SIZE = 10;							// per programs reqs
const int COMM_BUF_SIZE = 512;					// handle + "> " + input
const int INIT_MSG_SIZE = 7;						// corresponds to INIT_MSG & ACK_MSG lengths
const char INIT_MSG[] = "PORTNUM";
const char ACK_MSG[] = "ACKPROC";

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
	fprintf(stderr,"USAGE: %s server_hostname port\n", name);
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
void get_handle(char *handle) {

	printf("Enter handle (10 chars max, no spaces): ");
	scanf("%s", handle);	
	clean_stdin();
	printf("Welcome %s. Type \\quit to quit chatting.\n\n", handle);	

}

/*
 * simple helper to print a prompt for user input message and read input to buffer
 * prereqs: memory allocated for buffer, handle; handle populated with value
 * returns: none. alters data stored in buffer
 */
void prompt(char *handle, char *buffer) {

	printf("%s> ", handle);																				// display handle as prompt
	fgets(buffer, INPUT_BUF_SIZE, stdin);													// get msg from user

}

/*
 * function to perform setup of TCP socket
 * prereqs: hostname and port number are valid
 * returns: file descriptor of established socket (int)
 */
int setup_socket(char *hostname, int portNumber) {

	int 		socketFD;
	struct 	sockaddr_in serverAddress;
	struct 	hostent* serverHostInfo;
	
	memset((char*)&serverAddress, '\0', sizeof(serverAddress));		// Clear out the address struct
	serverAddress.sin_family = AF_INET; 													// Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); 									// Store the port number
	serverHostInfo = gethostbyname(hostname);											// Convert hostname into a special form of address
	
	if (serverHostInfo == NULL) { 																// see if that host exists, if not, die
		fprintf(stderr, "chatclient: ERROR, no such host: %s. Exiting...\n", hostname);
		exit(1);
	}
	// copy in the address
	memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)serverHostInfo->h_addr, serverHostInfo->h_length);

	socketFD = socket(AF_INET, SOCK_STREAM, 0);										// Create the socket
	if (socketFD < 0) {																						// verify that socket creation did not produce an error
		error("chatclient: ERROR opening socket. Exiting...");
		exit(1);
	}
	
	// Connect socket to addr of server
	if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
		error("chatclient: ERROR connecting. Exiting...");
		exit(1);
	}
	return socketFD;

}

/*
 * performs initial connection request and recieves acknowledgement
 * prereqs: socketFD is a valid socket
 * returns: none
 */
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

/*
 * simple helper function to pass along quit request to server
 * prereqs: buffer == "\quit", socketFD is an open connection
 * returns: none
 */
void send_close(int socketFD, char *buffer) {

	if( send( socketFD, buffer, INPUT_BUF_SIZE, 0 ) < 0 )						// send quit message to server
			error( "chatclient: ERROR writing to socket" );	

}

int main( int argc, char *argv[] ){ 															// argv[0] = exe, argv[1] = hostname, argv[2] = port

	int 		socketFD, charsWritten, charsRead;
	char 		input_buffer[INPUT_BUF_SIZE];
	char		comm_buffer[COMM_BUF_SIZE];
	char 		handle[HANDLE_SIZE];

	if ( argc < 3 ) { 																							// program, hostname, port
		usage( argv[0] );
		exit(0); 
	}

	get_handle( handle );																						// get user's desired handle
	socketFD = setup_socket( argv[1], atoi(argv[2]) );							// setup TCP socket
	setup_connection( socketFD );																		// setup connection with server

	while( 1 ) {
		memset( input_buffer, '\0', INPUT_BUF_SIZE );									// Clear out the input buffer	
		memset( comm_buffer, '\0', COMM_BUF_SIZE );										// Clear out the comm buffer	
		prompt( handle, input_buffer );																// get msg from user
		if( strstr(input_buffer, "\\quit") ) {												// test for quit request from user. NOTE: must use strstr
			printf("Quit acknowledged. Exiting...\n");
			send_close(socketFD, input_buffer);													// tell server to close the connection
			break;																											// stop transmission loop
		}
		strcat( comm_buffer, handle );																// assemble msg (format: handle + "> " + input)
		strcat( comm_buffer, "> " );
		strcat( comm_buffer, input_buffer );
		if( send( socketFD, comm_buffer, COMM_BUF_SIZE, 0 ) < 0 )			// send message to server
			error( "chatclient: ERROR writing to socket" );

		memset( comm_buffer, '\0', COMM_BUF_SIZE );										// Clear out the comm buffer

		if( recv( socketFD, comm_buffer, COMM_BUF_SIZE - 1, 0 ) < 0 ) // Read data from the socket, leaving \0 at end
			error("chatclient: ERROR reading from socket");
		if( strcmp( comm_buffer, "\\quit" ) == 0 ) {									// check for quit command from server
			printf("Connection terminated by server. Exiting...\n");
			break;
		}
		printf( "%s\n", comm_buffer );																// print the recv'd data
	}
	
	close(socketFD);																								// close the socket before exit
	return 0;																												

}
