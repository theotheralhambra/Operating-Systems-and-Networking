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
char	*CLIENT_ID = "dec!!";
const int BUF_SIZE = 1028;

// fn prototypes
void 	error(const char *msg);
int 	readfile(const char *filename);
int		parse(int size);

// globals
char 	*file_buffer;

int main(int argc, char *argv[]){ 															// argv[0] = exe, argv[1] = ciphertext, argv[2] = key, argv[3] = port

	int 		socketFD, portNumber, charsWritten, charsRead;
	struct 	sockaddr_in serverAddress;
	struct 	hostent* serverHostInfo;
	char 		buffer[BUF_SIZE];
	int			txt_size, key_size;
	char		sizestr[10];
	char		*plaintext;

	if (argc < 4) { 																							// Check usage & args
		fprintf(stderr,"USAGE: %s ciphertext key port\n", argv[0]);	// %s hostname port\n", argv[0]); 
		exit(0); 
	}

	// Set up the server address struct
	memset((char*)&serverAddress, '\0', sizeof(serverAddress));		// Clear out the address struct
	portNumber = atoi(argv[3]);																		// Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; 													// Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); 									// Store the port number
	serverHostInfo = gethostbyname("localhost");									// Convert localhost into a special form of address

	if (serverHostInfo == NULL) { 																// this should not be tripping unless something bad is going on, host is fixed as localhost
		fprintf(stderr, "OTP_DEC: ERROR, no such host: localhost\n");// die screaming
		exit(0);
	}

	memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)serverHostInfo->h_addr, serverHostInfo->h_length);// Copy in the address

	// Set up the socket
	socketFD = socket(AF_INET, SOCK_STREAM, 0);										// Create the socket
	if (socketFD< 0) 
		error("OTP_DEC: ERROR opening socket");
	
	// Connect to server
	if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0)// Connect socket to addr
		error("OTP_DEC: ERROR connecting");

	// tell the server who we are
	charsWritten = send(socketFD, CLIENT_ID, 5, 0);								//write CLIENT_ID to server
	if (charsWritten < 0) 
		error("OTP_DEC: ERROR writing to socket");
	// see if we are clear to proceed
	memset(buffer, '\0', BUF_SIZE);																// Clear out the buffer													-- BABY
	charsRead = recv(socketFD, buffer, BUF_SIZE - 1, 0); 					// Read data from the socket, leaving \0 at end
	if (charsRead < 0) 
		error("OTP_DEC: ERROR reading from socket");
	//printf("received %s from server\n", buffer);
	// see if the server says we are clear to proceed
	if (strcmp(buffer, "1") != 0) {																// server will send 0 if process is not clear to proceed
		fprintf(stderr, "OTP_DEC: ERROR connecting to OTP_DEC_D: unauthorised");	// if we are denied, print error and die
		exit(EXIT_FAILURE);
	}

	// we are a valid client - start transmissions
	/***** 1. send ciphertext *****/
	txt_size = readfile(argv[1]);																	// get size and contents of ciphertext
	// tell server how many chars to expect
	memset(sizestr, '\0', 10);																		// clear out the size container
	sprintf(sizestr, "%d\0", txt_size);														// convert size to string for transmission
	charsWritten = send(socketFD, sizestr, sizeof(sizestr), 0);		// write to server
	if (charsWritten < 0) 
		error("OTP_DEC: ERROR writing to socket");
	// get server ack
	memset(buffer, '\0', BUF_SIZE);																// Clear out the buffer													-- BABY
	charsRead = recv(socketFD, buffer, BUF_SIZE - 1, 0); 					// Read data from the socket, leaving \0 at end
	if (charsRead < 0) 
		error("OTP_DEC: ERROR reading from socket");
	// send ciphertext
	charsWritten = 0;																							// prepare counter
	while (charsWritten < txt_size) {															// send the whole msg
		charsWritten += send(socketFD, file_buffer + charsWritten, txt_size - charsWritten, 0);		// write to server
		if (charsWritten < 0) 
			error("OTP_DEC: ERROR writing to socket");
	}
	/***** END CIPHERTEXT COMM *****/
	
	// get server ack
	memset(buffer, '\0', BUF_SIZE);																// Clear out the buffer													-- BABY
	charsRead = recv(socketFD, buffer, BUF_SIZE - 1, 0); 					// Read data from the socket, leaving \0 at end
	if (charsRead < 0) 
		error("OTP_DEC: ERROR reading from socket");
	
	free(file_buffer);																						// free the buffer with the plaintext in it	

	/***** 2. send key *****/
	key_size = readfile(argv[2]);																	// get size and contents of key
	if (key_size < txt_size) {
		fprintf(stderr, "OTP_DEC: ERROR key of insufficient size\n");
		exit(EXIT_FAILURE);
	}
	memset(sizestr, '\0', 10);																		// clear out the size container
	sprintf(sizestr, "%d\0", key_size);														// convert size to string for transmission
	charsWritten = send(socketFD, sizestr, sizeof(sizestr), 0);		// write to server
	if (charsWritten < 0) 
		error("OTP_DEC: ERROR writing to socket");
	// get server ack
	memset(buffer, '\0', BUF_SIZE);																// Clear out the buffer													-- BABY
	charsRead = recv(socketFD, buffer, BUF_SIZE - 1, 0); 					// Read data from the socket, leaving \0 at end
	if (charsRead < 0) 
		error("OTP_DEC: ERROR reading from socket");
	// send key
	charsWritten = 0;																							// prepare counter
	while (charsWritten < key_size) {															// send until we send it all
		charsWritten += send(socketFD, file_buffer + charsWritten, key_size - charsWritten, 0);			// write to server
		if (charsWritten < 0) 
			error("OTP_DEC: ERROR writing to socket");
	}
	/***** END KEY COMM *****/

	// prepare to get the plaintext back
	free(file_buffer);																						// free the buffer with the key text in it
	plaintext = (char *)malloc(txt_size * sizeof(char));					// request memory for ciphertext
	memset(plaintext, '\0', txt_size);
	charsRead = 0;																								// reset charsRead
	// get back the plaintext
	while (strstr(plaintext, "!!") == NULL) {
		memset(buffer, '\0', BUF_SIZE);															// Clear out the buffer
		charsRead += recv(socketFD, buffer, BUF_SIZE - 1, 0); 			// Read data from the socket, leaving \0 at end
		strcat(plaintext, buffer);																	// add rec'd text to complete text
		if (charsRead <= 0) 
			error("OTP_DEC: ERROR reading from socket");
	}
	// fix the rec'd msg and print
	plaintext[txt_size-4] = '\n';																	// get rid of the transmission terminators and replace with newline and null terminator
	plaintext[txt_size-3] = '\0';
	printf("%s", plaintext);																			// print out the plaintext

	// cleanup
	free(plaintext);																							// free ciphertext memory
	close(socketFD);// Close the socket

	return 0;
}

/*
 * basic error printing helper
 * prereqs: none
 * returns: none
 */
void error(const char *msg) { 

	perror(msg); 
	exit(0); 

}

/*
 * simple function to read the contents of a file and convert to a string
 * prereqs: file_buffer has been declared as of type char *
 * returns: the size of the file read in number of chars
 */
int readfile(const char *filename) {

	int 		plaintextFD;
	struct	stat st;
	int 		file_size;
	char		*newline_ptr;
	
	stat(filename, &st);																			//get info on file
	file_size = st.st_size + 3;																//extract the size from the stat struct, add 3 for !!\0 at the end
	file_buffer = (char *)malloc(file_size * sizeof(char));		//make file_buffer big enough to hold the whole file
	
	//DEBUG//printf("number of chars in %s: %d\n", filename, file_size);
	plaintextFD = open(filename, O_RDONLY);										//open the file
	if (read(plaintextFD, file_buffer, file_size) == -1) {
		fprintf(stderr, "ERROR reading file %s\n", filename);
		exit(EXIT_FAILURE);
	}
	newline_ptr = strstr(file_buffer, "\n");									// find the newline - it should always be there
	if (newline_ptr != NULL) {																
		strncpy(newline_ptr, "!", 1);														// if we found it, replace with !
	}
	strcat(file_buffer, "!!\0");															// append !!\0 to the string (use double-! just to be safe)
	//DEBUG//printf("contents of %s:\n%s\n", filename, file_buffer);

	return file_size;

}

/*
 * NOT USED IN OTP_DEC.c
 * n/a
 * n/a
 */
int parse(int size) {
	
	int i;
	int c;
	// 	plaintext can only be [A..Z] or ' ' 
	for (i = 0; i < size; i++) {
		c = (int)file_buffer[i];
		if (c != 32 && c < 65 || c > 90)
			return 1;
	}
	
	return 0;

}
