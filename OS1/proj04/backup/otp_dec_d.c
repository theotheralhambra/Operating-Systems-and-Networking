#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

void error(const char *msg);

int main(int argc, char *argv[]) { //argv[0] = opt_end_d, argv[1] = port
	
	int 			listenSocketFD, connectionFD, portNum, charsRead;
	socklen_t sizeOfClientInfo;
	char			buffer[256];
	struct 		sockaddr_in serverAddress, clientAddress;

	if (argc < 2) {												// error on syntax - catches missing port
		fprintf(stderr, "USAGE: %s port\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	//setup addr struct for server
	memset((char *)&serverAddress, '\0', sizeof(serverAddress)); //clear addr
	portNum 											= atoi(argv[1]);	//get the port #
	serverAddress.sin_family 			= AF_NET;					//change for localhost??
	serverAddress.sin_port 	 			= htons(portNum);	//store port #
	serverAddress.sin_addr.s_addr = INADDR_ANY;			//change to limit to otp_enc??

	//listen on argv[1] port/socket
	//	- localhost as target ip addr/host
	//setup the socket			AF_UNIX for same machine IPC??
	listenSocketFD = socket(AF_INET, SOCK_STREAM, 0);//create the socket
	if (listenSocketdFD < 0)										
		error("ERROR opening socket");
	
	//enable the socket to begin listening
	if (bind(listenSocketFD, (struct sockaddr *)&serverAddress, 
														sizeof(serverAddress)) < 0) //connect to port
		error("ERROR on bind");
	listen(listenSocketFD, 5);												//flip socket on - queue = 5
	
		
	//listen on argv[1] port/socket
	//	- localhost as target ip addr/host
	//must accept up to 5 concurrent/simultaneous connections
	//try to connect
	//error on network error (stderr) i.e. port unavailable
	//accept()
	//fork
	//in child - check if communicating with otp_enc
	//in child - receive plaintext
	//in child - receive key
	//in child - do encryption
	//	note: key must be at leat as big as the plaintext! test [strlen()]
	//in child - send back the ciphertext via same socket
	 
	return 0;
	
}

void error(const char *msg) {

	fprintf("%s\n", msg);

}
