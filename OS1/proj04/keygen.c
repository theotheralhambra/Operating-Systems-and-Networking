/*
 * keygen.c - a simple function to print a specified (as cmd line argument)
 *   number of random chars in range [A..Z] plus spaces
 * Programmer: Rick Menzel (menzelr@oregonstate.edu)
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


void syntax();
void rand_char();

int main(int argc, char *argv[]) {//arg 1 = "keygen", arg 2 = keylength

	int i;													//iterator
	int keylen;											//number of letters to generate

	if (argc != 2) {								//incorrect # args -> bad invocation
		syntax();											//politely remind user how to use this thing
		exit(EXIT_FAILURE);						//die screaming
	}
	
	keylen = atoi(argv[1]);					//convert keylength arg to int
	srand( time(NULL) );						//initilaize rand()
	
	for(i = 0; i < keylen; i++) {
		rand_char();									//gen chars until we hit the requested #
	}
	printf("\n");										//terminal newline
	
	return 0;

}

/*
 * simple function to print usage if user has lost their mind
 * prereqs: none
 * returns: none
 */
void syntax() {

	fprintf(stderr, "error in program invocation: ");
	fprintf(stderr, "keygen [keylength]\n");

}

/*
 * function to generate an print a random char in range [A..Z] as well as spaces
 * prereqs: srand() called to initialize rand()
 * returns: none
 */
void rand_char() {

	int 	limit = 26;							//26 chars (A-Z) + 0 = space
	int 	div   = RAND_MAX/(limit + 1);//divide the range into 27 chunks
	int 	r;											//random recepticle
	char 	c;											//char recepticle

	do {													//this method avoids skewing distribution
		r = rand() / div;						//gen random in range [0..26] inclusive
	} while (r > limit);					//try again if out of range

	r += 64;											//add offset for ascii conversion to [A..Z]
	
	if (r == 64) {								//not in range [A..Z] -> char is a space
		c = ' ';
	} else {
		c = (char)r;								//get the char corresponding to that ascii value
	}
	printf("%c", c);							//spit it out

}
