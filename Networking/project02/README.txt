This is a README for CS372 Programming Assignment #2
Covering ftserver.c and ftclient.py
These programs, including makefile, were written and tested on flip1.engr.oregonstate.edu and flip3.engr.oregonstate.edu

Author: Rick Menzel
Email: menzelr@oregonstate.edu

COMPILING:
Compile ftserver.c from the Linux command line using the make command in conjunction with the included makefile
or using gcc with the following syntax: gcc -o ftserver ftserver.c

ftclient.py requires no compilation

RUNNING:
1. run ftserver from the Linux command line as:

		./ftserver <sevre_port>

2. run ftclient from the Linux command line as either:

		./ftclient.py <server_host> <server_port> <command> <data_port> <filename>
				OR
		python ftclient.py <server_host> <server_port> <command> <data_port> <filename>

		valid commands are:

			-g : get files
			-l : list CWD

	*	 note that these arguments MUST be in the above order	

	*	 note that hostname may be either the hostname reported by ftserver at runtime (likely flipX.engr.oregonstate.edu)
		 or "localhost" (no quotes) if both client and server are being run on the same host. 

	* note that the filename argument is optional (if included will be ignored) when using the command -g

	*  note that the maximum file size is 100 MiB for this system

REFERENCES:
* https://docs.python.org/2/howto/sockets.html
* my own programs from assignment 1
