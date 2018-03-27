/*
 * rtserver.c
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
const int 	COMM_BUF_SIZE = 128;
const int 	FILE_BUF_SIZE = 104857600;		// per instructor this is 100 MB = 100,000,000 Bytes; sizeof(char) = 1 Byte; 100 MB on flip = 104857600 chars

/*
 * simple helper to print errors to the terminal
 * prereqs: none
 * returns: none
 */
void error(const char *msg) { 

	perror(msg); 
	//exit(0); 

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
 * simple function to pull leftover junk (newline) out of stdin after scanf(). Req due to fflush() behaviour being undefined on non-stdout
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
 * function to perform setup of TCP data socket
 * prereqs: port number and hostname are valid
 * returns: file descriptor of established socket (int)
 */
int setup_data_socket(char *hostname, int portNumber) {
	
	int 		dataSocketFD;
	struct	sockaddr_in dataServerAddress;
	struct 	hostent* dataServerHostInfo;
	
	memset((char*)&dataServerAddress, '\0', sizeof(dataServerAddress));// clear out the address struct
	dataServerAddress.sin_family = AF_INET;												// create a network capable socket
	dataServerAddress.sin_port = htons(portNumber);								// store the port number
	dataServerHostInfo = gethostbyname(hostname);									// convert hostname into a special form of address
	
	if (dataServerHostInfo == NULL) {															// verify that the host exists
		fprintf(stderr, "ftserver: ERROR cannot establish data connection, no such host: %s.\n", hostname);
		return -1;
	}

	sleep(1);																											// wait for the client to catch up NOTE: critical on localhost

	// copy in the address 
	memcpy((char*)&dataServerAddress.sin_addr.s_addr, (char*)dataServerHostInfo->h_addr, dataServerHostInfo->h_length);
	dataSocketFD = socket(AF_INET, SOCK_STREAM, 0);								// create the socket
	if (dataSocketFD < 0) {																				// verify that creation did not produce an error
		error("ftserver: ERROR opening data connection socket.");
		return -1;
	}	

	// connect socket to addr of server
	if (connect(dataSocketFD, (struct sockaddr*)&dataServerAddress, sizeof(dataServerAddress)) < 0) {
		error("ftserver: ERROR connecting for data connection.");
		return -1;
	}
	printf("ftserver: established data connection with client.\n");

	return dataSocketFD;
	
}

/*
 * simple function to figure out the server's hostname
 * prereqs: none
 * returns: char array containing FQDN
 */
char *get_local_hostname() {
	
	int  s = 64;
	char *hostname = (char *)malloc(sizeof(char) * s);							// set up the info object
	memset(hostname, '\0', s);																			// clear it out
	
	gethostname(hostname, s);																				// get the name
	return hostname;
}

/*
 * simple function to pull names of files in cwd and add them to a buffer for further use
 * prereqs: memory allocated for buffer
 * returns: none; alters contents of buffer
 */
void list_files(char *buffer) {

	DIR			*dptr = opendir(".");																		// open current directory
	struct	dirent *fptr = NULL;																		// set file ptr to null

	memset(buffer, '\0', sizeof(buffer));														// clear out send buffer
	while ((fptr = readdir(dptr)) != NULL) {												// while there are more files in dir
		// need to omit directories here??
		if (strcmp(fptr->d_name, ".") != 0 && strcmp(fptr->d_name, "..") != 0) // don't list . and ..
			strcat(buffer, fptr->d_name);																// add the file name to the send buffer
			strcat(buffer, "\n");
	}

}

/*
 * simple function to write the contents of buffer to a specified TCP connection; can handle buffers of arbitrary length
 * prereqs: connectionFD is a valid TCP connection; buffer allocated and populated
 * returns: none
 */
void send_data(int connectionFD, char *buffer) {
	
	int			charsWritten = 0, i;
	// send
	printf("ftserver: sending data...\n");													// verbose
	while (charsWritten < strlen(buffer)) {													// write until the entire buffer has been sent
		i = send(connectionFD, buffer + charsWritten, strlen(buffer) - charsWritten, 0);
		if (i < 0)
			error("ftserver: ERROR writing to socket");	
		charsWritten += i;
	}

} 

/*
 * simple helper function to determine if a specified target file exists and is a regular file
 * prereqs: target allocated and populated
 * returns: 1 if the files exists and is a valid file, 0 if does not exist or is directory
 */
int valid_file(char *target) {
	DIR			*dptr = opendir(".");																		// open current directory
	struct	dirent *fptr = NULL;																		// set file ptr to null
	int			charsWritten = 0, i;
	struct stat file_stat;

	while ((fptr = readdir(dptr)) != NULL) {												// while there are more files in dir
		if (strcmp(fptr->d_name, target) == 0) {											// if the file exists 
			stat(target, &file_stat);																		// stat the file
			return S_ISREG(file_stat.st_mode);													// will return 0 for directories
		}
	}
	return 0;																												// file not found

}

/*
 * helper function to take a command string stored in buffer and separate data into useful chunks
 * prereqs: memory allocated for all strings; buffer populated
 * returns: 1 for valid list, 2 for valid get, 3 for get on bad file, 0 otherwise
 */
int parse_command(char *buffer, char *command, char *filename, char *dataport) {
	
	strcpy(command, strtok(buffer, " "));														// split up the command string
	strcpy(dataport, strtok(NULL, " "));
	strcpy(filename, strtok(NULL, " "));

	//find out what we have
	if (strcmp(command, "-l") == 0 && dataport != NULL) {						// is valid list request
		return 1;
	} else if (strcmp(command, "-g") == 0 && dataport != NULL && valid_file(filename)) { // is valid get request
		return 2;
	} else if (strcmp(command, "-g") == 0 && dataport != NULL) {		// get reqiest on bad file name
		return 3;
	} else {																												// something bad happened
		return 0;	
	}

}

/*
 * simple function to read the contents of filename and store it in buffer
 * prereqs: buffer is allocated and big enough ()does not check as this is ensured elsewhere in program IAW spec
 * returns: none; alters buffer
 */
void read_file(char *filename, char *buffer, int buf_size) {
	
	int fd;
	
	memset(buffer, '\0', buf_size);																	// clear out the buffer
	fd = open(filename, O_RDONLY);																	// open the file
	
	if (read(fd, buffer, buf_size) == -1) {													// read the file into buffer
		error("ftserver: ERROR reading file");												// check for errors
	} 

}

int main( int argc, char *argv[] ){ 															// argv[0] = exe, argv[1] = hostname, argv[2] = port

	int 			controlPort, socketFD, dataFD, controlConnectionFD, dataConnectionFD, charsWritten, charsRead;
	socklen_t	sizeOfClientInfo;
	struct 		sockaddr_in clientAddress;
	char			*command;																							// see malloc's below
	char			*filename;
	char			*dataport;
	char			*comm_buffer;
	char			*file_buffer;
	char			clientName[512]; 
	
	if ( argc < 2 ) { 																							// check for valid call format; program, port
		usage( argv[0] );
		exit(0); 
	}

	controlPort = atoi(argv[1]);
	socketFD = setup_socket(controlPort);														// setup TCP socket
	listen(socketFD, 5);																						// listen on that socket; baclog size = 5
	
	while( 1 ) {

		comm_buffer = (char *)malloc(sizeof(char) * COMM_BUF_SIZE);		// allocate memory for network buffers
		file_buffer = (char *)malloc(sizeof(char) * FILE_BUF_SIZE);
		command 		= (char *)malloc(sizeof(char) * 2);
		filename 		= (char *)malloc(sizeof(char) * (COMM_BUF_SIZE - 2) / 2);
		dataport		= (char *)malloc(sizeof(char) * (COMM_BUF_SIZE - 2) / 2);

		printf("ftserver: host '%s' listening on port %d\n", get_local_hostname(), controlPort);			// verbose
		//Accept a connection
		sizeOfClientInfo = sizeof(clientAddress);
		controlConnectionFD = accept(socketFD, (struct sockaddr *)&clientAddress, &sizeOfClientInfo); // accept connection
		if (controlConnectionFD < 0) {																// verify that accept did not produce an error
			error("ftserver: ERROR on accept.");
		} else {
			memset(clientName, '\0', sizeof(clientName));								// on valid accept get the client name
			getnameinfo((struct sockaddr *)&clientAddress, sizeOfClientInfo, clientName, sizeof(clientName), NULL, 0, NI_NAMEREQD);
			printf("ftserver: opened control connection with client '%s' on port %d\n", clientName, atoi(argv[1]));
		}
	
		// get command from client
		memset(comm_buffer, '\0', COMM_BUF_SIZE);
		charsRead = recv(controlConnectionFD, comm_buffer, COMM_BUF_SIZE - 1, 0); // read the client's message from socket
		if (charsRead < 0) 																						// verify that read did not produce an error
			error("ftserver: ERROR reading from socket");

		// work on the command
		switch(parse_command(comm_buffer, command, filename, dataport)) {
			case 1:																											// valid list reques
				list_files(file_buffer);																	// get list
				dataFD = setup_data_socket(clientName, atoi(dataport));		// setup data conection
				send_data(dataFD, file_buffer);														// send list
				break;
			case 2:																											// valid get request
				dataFD = setup_data_socket(clientName, atoi(dataport));		// setup data connection
				read_file(filename, file_buffer, FILE_BUF_SIZE); 					// read the file
				send_data(dataFD, file_buffer);														// send the file
				printf("ftserver: transfer complete\n");									// verbose
				break;
			case 3:																											// get request with bad filename
				printf("ftserver: ERROR invalid file name specified.\n");	// verbose
				dataFD = setup_data_socket(clientName, atoi(dataport));		// set up and kill data connection (client will hang waiting for inc commo)
				send_data(dataFD, "ERROR");
				if (send(controlConnectionFD, "ftserver: ERROR invalid file name specified.", 44, 0) < 0)	// send invalid file msg on control conn
					error("ftserver: ERROR writing to socket.");
				break;
			default:																										// invalid call
				printf("ftserver: ERROR invalid or malformed command received.\n"); // verbose
				dataFD = setup_data_socket(clientName, atoi(dataport));		// set up and kill data connection (client will hang waiting for inc commo)
				send_data(dataFD, "ERROR");
				if (send(controlConnectionFD, "ftserver: ERROR invalid or malformed command specified.", 55, 0) < 0) // send bad call msg on control conn
					error("ftserver: ERROR writing to socket.");
	
		}// end switch()
		close(dataFD);																								// close data conn
		printf("ftserver: closed data connection with client.\n");		// verbose
		close(controlConnectionFD);																		// close the server side of the controlconnection
		printf("ftserver: closed control connection with client on port %d\n", atoi(argv[1])); // verbose
		free(comm_buffer);																						// free buffers
		free(file_buffer);
		free(command);
		free(filename);
		free(dataport);
		
	}
	
	close(socketFD);																								// close the listening socket before exit
	return 0;																												

}
