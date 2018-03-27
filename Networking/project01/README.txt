This is a README for CS372 Programming Assignment #1
Covering chatclient.c and chatserve.py
These programs, including makefile, were written and tested on flip3.engr.oregonstate.edu

Author: Rick Menzel
Email: menzelr@oregonstate.edu

COMPILING:
Compile chatclient.c from the Linux command line using the make command in conjunction with the included makefile
or using gcc with the following syntax: gcc -o chatclient chatcleint.c

chatserve.py requires no compilation

RUNNING:
1. run chatserve from the Linux command line as either:

		./chatserve.py portnumber
				OR
		python chatserve.py portnumber

2. run chatclient from the Linux command line as:

		./chatclient hostname portnumber

	 note that hostname may be either the hostname reported by chatserve at runtime (likely flipX.engr.oregonstate.edu)
	 or "localhost" (no quotes) if both chatclient and chatserve are being run on the same host
3. in chatclient, enter the desired handle. chatserve's handle is hardcoded to "server"
4. after initial connection request, users may take turns messaging back and forth. Note that the first message will be 
	 sent from chatclient. The program is configured this way to avoid the first message in every communication being the 
	 server asking what the client wants, i.e. the initiating party sends the first text message.
5. either chatclient or chatserve may type "\quit" (no quotes) to quit at any time. If the user wishes to exit the program 
	 for chatserve, they should use keyboard interrupt (Ctrl+C) 
6. note that the maximum length of message one may enter is 500 characters. The transmitted message, once concatenated 
	 with the user's handle and formatting characters, then comes to 512 characters. This was done in accordance with 
	 interpretation of program specifications and to better suit the recv routines (man pages state that message length 
	 should be a power of 2)

REFERENCES:
* https://docs.python.org/2/howto/sockets.html
