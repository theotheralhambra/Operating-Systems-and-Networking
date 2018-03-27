/*
 *Program: buildrooms.c
 *Programmed by: Rick Menzel (menzelr@oregonstate.edu)
 *Class: CS344
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

//global vars
struct 	Room {
	char	name[12];
	int		numCon;
	char	connections[6][12];
	char 	type[9];
};
struct Room roomList[7];
char	 workingNames[7][12];

//fn prototypes
void 		makeDir();
void		writeFile();
void		genNames();
int			isgraphFull();
int			canAddConnection(struct Room rm);
int 		connectionExists(struct Room rmA, struct Room rmB);
void		connectRooms(int A, int B);
void		addRandomConnection();
int			needConnections();

//constants
const char* NAME_LIST[10] = { "Antechamber", 
										 				 	"Corridor", 
														 	"Shrine", 
															"Reliquary", 
															"Dungeon",
															"Tomb",
															"Library",
															"Lab",
															"Barracks",
															"Cellar" };
const char* DIR_NAME = "menzelr.rooms.";
const int		VERBOSE  = 1; 

main(){

	int 	r; //random number recepticle
	int		i, j; //iterator	
	//int		select[6] = {0, 0, 0, 0, 0, 0};

	int		startRoom = 0;
	int		endRoom = 0;
	struct Room currRoom;
	//char	currType[9];

	//seed rand()
	srand( time(NULL) );

	//create directory
	makeDir();

	//determine start and end rooms
	startRoom = rand() % 7;
	do{
		endRoom = rand() % 7;
	} while (endRoom == startRoom);

	genNames();

	//populate roomList (no connections at this point)
	for (i = 0; i < 7; i++){
		//reset currRoom object
		if (i == startRoom) {											//populate room type
			strcpy(currRoom.type, "1ST_ROOM");			//is first
		} else if (i == endRoom) {
			strcpy(currRoom.type, "END_ROOM");			//is last
		} else {
			strcpy(currRoom.type, "MID_ROOM");			//neither first nor last
		}
		strcpy(currRoom.name, workingNames[i]);		//populate room name
		currRoom.numCon = 0;											//reset room connection count
		for (j = 0; j < 6; j++) {									//reset connection array
			strcpy(currRoom.connections[j], "0");
		}
	
		//add currRoom object	to roomList
		roomList[i] = currRoom;
		//printf( "roomList[%d]: \nName: %s\nType: %s\n# Connections: %d\n\n", i, roomList[i].name, roomList[i].type, roomList[i].numCon );
	}

	//work on room connections
	do {
		//printf("adding\n");
		addRandomConnection();
	} while ( needConnections() );	

	//for (i=0;i<7;i++)
	//	printf( "roomList[%d]: \nName: %s\nType: %s\n# Connections: %d\n\n", i, roomList[i].name, roomList[i].type, roomList[i].numCon );

	//write Room object array to file
	writeFile();

	return 0;

}

/*
 * function to create a directory inside cwd
 * prereqs: ability to write within cwd, free space (trivial)
 * returns: none
 */ 
void makeDir(){
	
	struct 	stat st = {0};
	char		dir[21];	//len of DIR_NAME(14) + len of MAX_PID(7)
	//create dir name string with pid
	sprintf( dir, "%s%d", DIR_NAME, getpid() );

	if (stat( dir, &st) == -1) {															//see if dir exists
		if ( mkdir( dir, 0700 ) != 0 ){													//make dir
			printf( "When attempting: mkdir(%s, 0700)\n", dir );	//failure
			perror( "mkdir" );
			exit(1);
		} else {
			//printf( "Created Directory '%s'\n", dir );						//success
			return;
		}
	} else {																									//dir already exists
		printf( "When attempting: mkdir(%s, 0700)\n", dir );
		perror( "stat" );
		exit(1);
	}

}

/*
 * function to write genreated room data to a file within target dir
 * prereqs: target dir exists, free space (trivial)
 * returns: none
 */
void writeFile(){

	int			i, j;
	int			file_descriptor;
	ssize_t nwritten;
	char		file[38]; //14 + 7 + 12 + 5
	char		roomStub[] = "ROOM NAME: ";
	char		conStub[14];
	char		typeStub[] = "ROOM TYPE: "; 

	for (i = 0; i < 7; i++) {
		sprintf( file, "%s%d/%s_room", DIR_NAME, getpid(), roomList[i].name );
		//printf("%s\n", file);
		file_descriptor = open(file, O_WRONLY | O_TRUNC | O_CREAT, 0600);

		if (file_descriptor < 0) {
			printf("Could not open %s\n", file);
			perror("Error in writeFile()");
			exit(1);
		}
		//write "ROOM NAME: <room name>"
		write(file_descriptor, roomStub, strlen(roomStub) * sizeof(char) );
		nwritten = write( file_descriptor, roomList[i].name, 
											strlen(roomList[i].name) * sizeof(char) );
		nwritten = write( file_descriptor, "\n", strlen("\n") * sizeof(char) );
		for (j = 0; j < roomList[i].numCon; j++){
			//write "CONNECTION i: "
			sprintf( conStub, "CONNECTION %d: ", j+1 );
			write( file_descriptor, conStub, strlen(conStub) * sizeof(char) );
			//write "<room name>"
			write( file_descriptor,  roomList[i].connections[j], 
						strlen(roomList[i].connections[j]) * sizeof(char) );
		  write( file_descriptor, "\n", strlen("\n") * sizeof(char) );
		}
		//write "ROOM TYPE: <room type>"
		write ( file_descriptor, typeStub, strlen(typeStub) * sizeof(char) );
		write( file_descriptor,  roomList[i].type, 
					 strlen(roomList[i].type) * sizeof(char) );
		if ( write( file_descriptor, "\n", strlen("\n") * sizeof(char) ) == -1 ){
			fprintf(stderr, "Error writing to %\n", file);
			perror("Error in writeFile()");
			exit(1);
		}
		
	}

	return;

}

/*
 * function to generate unique room names
 * prereqs: NAME_LIST populated
 * returns: populated array of names (global var)
 */
void genNames(){

	int		select[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	int		r, i;
	int 	count = 0;
	//get 7 unique int values in range [1..10]
	while (count < 7) {
		r = rand() % 10 + 1;								//gen rand in [1..10]
		if (select[r-1] == 0) {							//see if selection arr is set
			select[r-1] = r;									//if not, set -> val is unique
			strcpy( workingNames[count], NAME_LIST[r-1] );	//copy name value
							//NOTE:think this is faster than two array lookups, can refactor
			count++;													//count up
		}
	}
	//for (i=0; i< 7; i++)
	//	printf("Name %d is %s\n",i,workingNames[i]);
	return;

}

/*
 * funtion to determine if all rooms have the required # of connections
 * prereqs: roomList populated
 * returns: 1 if all rooms have between 3 and 6 outbound connections,
 * 					false otherwise
 */
int isGraphFull(){

	int i;

	for (i = 0; i < 7; i++){
		if ( roomList[i].numCon < 3 || roomList[i].numCon > 6){
			return 0;
		}
	}
	return 1;	

}

/*
 *
 *
 *
 */
void connectRooms(int A, int B){

	int 	i;
		
	//if (roomList[a].numCon == 0) {
	//	strcpy( roomList[A].connections[0], roomList[B].name );
	//}

	for (i = 0; i <= roomList[A].numCon; i++) {

		if ( strcmp(roomList[A].connections[i], "0")  == 0 ){
			strcpy( roomList[A].connections[i], roomList[B].name );
			break;
		}
	}

	roomList[A].numCon++;
	return;

}

/*
 * function to determine whether a target room can acoomodate another connection
 * prereqs: properly populated Room object
 * returns: 1 if the room can take another connection, 0 otherwise
 */
int canAddConnectionFrom(struct Room rm){

	if (rm.numCon < 6) {
		return 1;
	} else {
		return 0;
	}

}

/*
 *
 *
 *
 */
int connectionExists(struct Room rmA, struct Room rmB){

	int 	i; 
	int 	x = rmA.numCon;
	char	A[12];
	char	B[12];

	strcpy(B, rmB.name);

	for (i = 0; i < x; i++){
		
		strcpy(A, rmA.connections[i]);
		if ( strcmp(A, B) == 0){
			return 1;
		}		

	}

	return 0;

}

/*
 *
 *
 *
 */
void addRandomConnection(){

	struct Room A;
	struct Room B;
	int 	 rA, rB;

	while(1){
		
		rA = rand() % 7;
		A = roomList[rA];										//get a random room	
		if (canAddConnectionFrom(A) == 1)		//if we can add a connection, keep it
			break;

	}

	do {

		rB = rand() % 7;
		B = roomList[rB];

	} while ( canAddConnectionFrom(B) == 0 || strcmp(A.name, B.name) == 0 || connectionExists(A, B) == 1);

	connectRooms(rA, rB);
	connectRooms(rB, rA);
	
	return;	

}

/*
 *
 *
 *
 */
int needConnections(){

	int i;

	for (i = 0; i < 7; i++){

		if (roomList[i].numCon < 3){
			return 1;
		}

	}

	return 0;

}
