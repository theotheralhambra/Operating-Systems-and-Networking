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
char	*CLIENT_ID = "enc@@"; //or dec@@
const int BUF_SIZE = 1028;

// fn prototypes
void 	error(const char *msg);
int 	readfile(const char *filename);
int		parse(int size);

// globals
char 	*file_buffer;

int main(int argc, char *argv[]){ 															// argv[0] = exe, argv[1] = plaintext, argv[2] = key, argv[3] = port

	int 		socketFD, portNumber, charsWritten, charsRead;
	struct 	sockaddr_in serverAddress;
	struct 	hostent* serverHostInfo;
	char 		buffer[BUF_SIZE];
	int			txt_size, key_size;
	char		sizestr[10];
	char		*ciphertext;

	if (argc < 4) { 																							// Check usage & args
		fprintf(stderr,"USAGE: %s plaintext key port\n", argv[0]);	// %s hostname port\n", argv[0]); 
		exit(0); 
	}

	// Set up the server address struct
	memset((char*)&serverAddress, '\0', sizeof(serverAddress));		// Clear out the address struct
	portNumber = atoi(argv[3]);																		// Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; 													// Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); 									// Store the port number
	serverHostInfo = gethostbyname("localhost");									// Convert localhost into a special form of address

	if (serverHostInfo == NULL) { 																// this should not be tripping unless something bad is going on, host is fixed as localhost
		fprintf(stderr, "CLIENT: ERROR, no such host: localhost\n");// die screaming
		exit(0);
	}

	memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)serverHostInfo->h_addr, serverHostInfo->h_length);// Copy in the address

	// Set up the socket
	socketFD = socket(AF_INET, SOCK_STREAM, 0);										// Create the socket
	if (socketFD< 0) 
		error("CLIENT: ERROR opening socket");
	
	// Connect to server
	if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0)// Connect socket to addr
		error("CLIENT: ERROR connecting");

	// tell the server who we are
	charsWritten = send(socketFD, CLIENT_ID, 5, 0);								//write CLIENT_ID to server
	if (charsWritten < 0) 
		error("CLIENT: ERROR writing to socket");
	// see if we are clear to proceed
	memset(buffer, '\0', BUF_SIZE);																// Clear out the buffer													-- BABY
	charsRead = recv(socketFD, buffer, BUF_SIZE - 1, 0); 					// Read data from the socket, leaving \0 at end
	if (charsRead < 0) 
		error("CLIENT: ERROR reading from socket");
	//printf("received %s from server\n", buffer);
	// see if the server says we are clear to proceed
	if (strcmp(buffer, "1") != 0) {																// server will send 0 if process is not clear to proceed
		error("CLIENT: ERROR connecting to SERVER: unauthorised");	// if we are denied, print error and die
		exit(EXIT_FAILURE);
	}

	// we are a valid client - start transmissions
	/***** 1. send plaintext *****/
	txt_size = readfile(argv[1]);																	// get size and contents of plaintext
	if (parse(txt_size)) {																				// check input for non-allowed chars
		fprintf(stderr, "ERROR non-allowed chars present in file %s\n", argv[1]);
		exit(EXIT_FAILURE);
	}
	// tell server how many chars to expect
	memset(sizestr, "\0", 10);																		// clear out the size container
	sprintf(sizestr, "%d\0", txt_size);														// convert size to string for transmission
	charsWritten = send(socketFD, sizestr, sizeof(sizestr), 0);		// write to server
	if (charsWritten < 0) 
		error("CLIENT: ERROR writing to socket");
	// get server ack
	memset(buffer, '\0', BUF_SIZE);																// Clear out the buffer													-- BABY
	charsRead = recv(socketFD, buffer, BUF_SIZE - 1, 0); 					// Read data from the socket, leaving \0 at end
	if (charsRead < 0) 
		error("CLIENT: ERROR reading from socket");
	// send plaintext
	charsWritten = send(socketFD, file_buffer, txt_size, 0);		// write to server
	if (charsWritten < 0) 
		error("CLIENT: ERROR writing to socket");
	/***** END PLAINTEXT COMM *****/
	
	// get server ack
	memset(buffer, '\0', BUF_SIZE);																// Clear out the buffer													-- BABY
	charsRead = recv(socketFD, buffer, BUF_SIZE - 1, 0); 					// Read data from the socket, leaving \0 at end
	if (charsRead < 0) 
		error("CLIENT: ERROR reading from socket");
	
	free(file_buffer);																						// free the buffer with the plaintext in it	

	/***** 2. send key *****/
	key_size = readfile(argv[2]);																	// get size and contents of key
	if (parse(key_size)) {																				// check key for non-allowed chars -- SHOULD NOT BE NECESSARY
		fprintf(stderr, "ERROR non-allowed chars present in file %s\n", argv[1]);
		exit(EXIT_FAILURE);
	}
	// tell server how many chars to expect
	memset(sizestr, "\0", 10);																		// clear out the size container
	sprintf(sizestr, "%d\0", key_size);														// convert size to string for transmission
	charsWritten = send(socketFD, sizestr, sizeof(sizestr), 0);		// write to server
	if (charsWritten < 0) 
		error("CLIENT: ERROR writing to socket");
	// get server ack
	memset(buffer, '\0', BUF_SIZE);																// Clear out the buffer													-- BABY
	charsRead = recv(socketFD, buffer, BUF_SIZE - 1, 0); 					// Read data from the socket, leaving \0 at end
	if (charsRead < 0) 
		error("CLIENT: ERROR reading from socket");
	// send key
	charsWritten = send(socketFD, file_buffer, key_size, 0);			// write to server
	if (charsWritten < 0) 
		error("CLIENT: ERROR writing to socket");
	/***** END KEY COMM *****/

	free(file_buffer);																						// free the buffer with the key text in it
	ciphertext = (char *)malloc(txt_size * sizeof(char));					// request memory for ciphertext
	charsRead = 0;																								// reset charsRead
	// get back the crypto
	while (strstr(ciphertext, "@@") == NULL) {
		memset(buffer, '\0', BUF_SIZE);															// Clear out the buffer
		charsRead += recv(socketFD, buffer, BUF_SIZE - 1, 0); 			// Read data from the socket, leaving \0 at end
		strcat(ciphertext, buffer);																	// add rec'd text to complete text
		if (charsRead <= 0) 
			error("CLIENT: ERROR reading from socket");
	}
	printf("ciphertext:\n%s\n", ciphertext);											// print out the ciphertext
	// cleanup
	free(ciphertext);																							// free ciphertext memory
	// Send message to server
	//charsWritten = send(socketFD, buffer, strlen(buffer), 0);			// Write to the server
	//if (charsWritten < 0) 
	//	error("CLIENT: ERROR writing to socket");
	//if (charsWritten < strlen(buffer)) 
	//	printf("CLIENT: WARNING: Not all data written to socket!\n");
	close(socketFD);// Close the socket

	return 0;
}

/*
 *
 *
 *
 */
void error(const char *msg) { 

	perror(msg); 
	exit(0); 

}

/*
 *
 *
 *
 */
int readfile(const char *filename) {

	int 		plaintextFD;
	struct	stat st;
	int 		file_size;
	char		*newline_ptr;
	
	stat(filename, &st);																			//get info on file
	file_size = st.st_size + 3;																//extract the size from the stat struct, add 3 for @@\0 at the end
	file_buffer = (char *)malloc(file_size * sizeof(char));		//make file_buffer big enough to hold the whole file
	
	//DEBUG//printf("number of chars in %s: %d\n", filename, file_size);
	plaintextFD = open(filename, O_RDONLY);										//open the file
	if (read(plaintextFD, file_buffer, file_size) == -1) {
		fprintf(stderr, "ERROR reading file %s\n", filename);
		exit(EXIT_FAILURE);
	}
	newline_ptr = strstr(file_buffer, "\n");									// find the newline - it should always be there
	if (newline_ptr != NULL) {																
		strncpy(newline_ptr, "@", 1);														// if we found it, replace with @
	}
	strcat(file_buffer, "@@\0");															// append @@\0 to the string (use double-@ just to be safe)
	//DEBUG//printf("contents of %s:\n%s\n", filename, file_buffer);

	return file_size;

}

/*
 *
 *
 *
 */
int parse(int size) {
	
	// 	plaintext can only be [A..Z] or ' ' 

	return 0;

}
