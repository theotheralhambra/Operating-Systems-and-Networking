/*
 *Program: adventure.c
 *Programmed by: Rick Menzel (menzelr@oregonstate.edu)
 *Class: CS344
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

//global vars
struct 	Room {
	char	name[12];
	int		numCon;
	char	connections[6][12];
	char 	type[9];
};
struct 	Room roomList[7];
char	 	dirName[22];
char	 	nameList[7][17];
char	 	timeFile[38];
pthread_mutex_t myMutex;

//fn prototypes
void		getDir();
void		openDir();
void		readFiles();
int			getFirstRoomIndex();
int			getEndRoomIndex();
int			getNextRoomIndex(int currRm, char *str);
void		printRoom(int index);
void*		genTime(void *arg);
void		printTime();

//constants
const char* DIR_NAME = "menzelr.rooms.";

int main(){

	int 	i, x;
	int 	stepCount = 0;									//number of steps
	int		stepList[50];										//list of steps taken
	int		currRoom, endRoom;							//current room and target
	char	inBuf[15];											//read buffer
	int		t_result, t_args;
	pthread_t thread;
	
	pthread_mutex_init(&myMutex, NULL);		//initialize mutex
	pthread_mutex_lock(&myMutex);					//take the squirrel
	t_result = pthread_create(&thread, NULL, &genTime, &t_args); //create thread for time generation

	for (i = 0; i < 50; i++) {						//zero out the step list
		stepList[i] = 0;		
	}
	memset(inBuf, 0, sizeof(inBuf));			//zero out the read buffer
	getDir();															//find most recent directory
	openDir();														//open that dir and get file names
	readFiles();													//read files

	currRoom = getFirstRoomIndex();				//figure out starting room
	endRoom = getEndRoomIndex();					//figure out end room

	do {																	//let's play a game
		
		printRoom(currRoom);								//show currRoom and connections
		printf("WHERE TO? >");							//prompt for input
		scanf("%s", inBuf);									//read input
		
		if (strcmp(inBuf, "time") == 0){		//do time stuff
			
			pthread_mutex_unlock(&myMutex);		//release the squirrel
			pthread_join(thread, NULL);				//move to the time gen thread
			pthread_mutex_lock(&myMutex);			//take back the squirrel
			t_result = pthread_create(&thread, NULL, &genTime, &t_args);//create another thread in case we call time again

			printTime();											//read file to print time

			printf("\nWHERE TO? >");					//re-prompt for input
			scanf("%s", inBuf);								//re-read input
			
		} 														
			
		x = getNextRoomIndex(currRoom, inBuf);//get the index of the next room

		if (x >= 0 && x < 7) {						//if in range, name is valid
			currRoom = x;										//set new currRoom if valid
			stepList[stepCount] = currRoom;	//add to path list
			stepCount++;										//increment path count (only if valid)
			printf("\n");
		} else {													//bad room name or other nonsense
			printf("\nHUH? I DON'T UNDERSTAND THAT ROOM. TRY AGAIN.\n\n");
		}

	} while (currRoom != endRoom);					//see if the current room is the end

	//victory stuff
	printf("YOU HAVE FOUND THE END ROOM. CONGRATULATIONS!\n");
	printf("YOU TOOK %d STEPS. YOUR PATH TO VICTORY WAS:\n", stepCount);
	for (i = 0; i < stepCount; i++) {
		printf("%s\n", roomList[stepList[i]].name);
	}

	return 0;

}

/*
 * function to get the name of the most recently modified directory within cwd
 * prereqs: dirName initialized as global var
 * returns: name of most recently modified directory in dirName (global var)
 */
void getDir(){
	
	DIR 		*dp = opendir(".");				//open current working directory (cwd)
	struct 	dirent *fptr = NULL;			//set file ptr to NULL
	struct 	stat dStat;								//initialize stat data container
	time_t 	latest = 0;								//initialize current latest time to 0
	char		dName[22];								//container for dir names

	memset(dName, 0, sizeof(dName));		//flush dName
	memset(dirName, 0, sizeof(dirName));//flush global dirName

	while ( (fptr = readdir(dp)) != NULL ) {								//while there is more stuff in cwd

		memset( &dStat, 0, sizeof(dStat) );										//flush	dStat
		
		if ( stat(fptr->d_name, &dStat) < 0 ){								//something bad happened while getting file info
			printf("Error getting file info for %s\n", fptr->d_name);	//print error msg
			exit(1);																									//die
		} else if ( (dStat.st_mode & S_IFDIR) != S_IFDIR) {		//check if file is a directory
			continue;																									//mot a directory->skip
		} else if (dStat.st_mtime > latest && strcmp(fptr->d_name, ".") != 0 && strcmp(fptr->d_name, "..")!= 0) { //time is more recent and dir is not . or ..
			strcpy(dName, fptr->d_name);												//set dName to dir name
			latest = dStat.st_mtime;														//update latest time seen
		}
	
	}
	
	closedir(dp);																						//close the directory
	strcpy(dirName, dName);																	//put the dir name in global var

	return;

}

/*
 * function to open most recently modified dir and get file names
 * prereqs: dirName initialized and populated
 * returns: list of names in nameList (global var)
 */
void openDir(){
	
	DIR 		*dp = NULL;																			//initialize directory ptr to NULL
	struct 	dirent *fptr = NULL;														//initialize file ptr to NULL
	int 		i, count = 0;

	for (i = 0; i < 7; i++) {																//flush nameList	
		memset(nameList[i], 0, sizeof(nameList[i]));
	}

	if (NULL == (dp = opendir(dirName) ) ) {								//cannot open dir
		printf("Cannot open input directory '%s'\n", dirName);//print error
		perror("Error in opeDir()");
		exit(1);																							//die
	} else {																								//succesfully opened

		while( (fptr = readdir(dp)) != NULL ) {								//get contents
			//do NOT read . , .. , or time file !!IMPORTANT!!
			if (strcmp(fptr->d_name, ".") != 0 && strcmp(fptr->d_name, "..") != 0 && strcmp(fptr->d_name, "currentTime.txt") != 0) {
				strcpy(nameList[count++], fptr->d_name);					//copy file names to nameList
			}
	
		}
		closedir(dp);																					//close directory
	}
	return;
}

/*
 * function to read the contents of Room files and deposit them into roomList (Room struct array)
 * prereqs: dirName generated
 * returns: populated list of Room structs (in global var rookmList)
 */
void readFiles(){

	int			file_descriptor;
	int			i, j, count;
	char		file[38];			//file name container
	char		readBuf[170];	//file read container (max calc to 168 chars)
	char		*swap = 0;		//line container
	char		item[15];			//data var container
	char		test[15];			//container for other data in file (determines EOF)
	char		arr[7][12];		//array to store items

	for (i = 0; i < 7; i++) {
		
		count = 0;																				//reset count
		memset(readBuf, 0, sizeof(readBuf));							//clear readBuf

		sprintf( file, "%s/%s", dirName, nameList[i] );		//generate file name
		file_descriptor = open(file, O_RDONLY);						//open file
	
		if( read(file_descriptor, readBuf, sizeof(readBuf)) == -1) {
			printf("Error reading file '%s'\n", file);			//print error
			perror("Error in readFiles()");
			exit(1);																				//die
		}

		swap = strtok(readBuf, "\n");											//get the first line - this is different bc strtok is weird
		sscanf(swap, "%*s %s %s", test, arr[count]);			//pull tokens
		count++;																					//inc count

		do{

			swap = strtok(NULL, "\n");											//get next line
			sscanf(swap, "%*s %s %s", test, arr[count]);		//pull tokens
			count++;																				//inc count
		
		} while( strcmp(test, "TYPE:") != 0);							//type is last line of file
		
		//copy array vals to struct
		strcpy(roomList[i].name, arr[0]);									//name
		roomList[i].numCon = count - 2;										//count - name - type => count -2 => # connections 
		for ( j = 1; j < count - 1 ; j++) {								//connections; starts at arr[1] (due to name) and ends at arr[count-1] (due to type)
			strcpy(roomList[i].connections[j-1], arr[j]);	
		}
		strcpy(roomList[i].type, arr[count-1]);						//type
	}
	return;
}

/*
* simple function to find which room is 1ST_ROOM
* prereqs: roomsList populated
* returns: index of 1ST_ROOM
*/
int getFirstRoomIndex(){

	int i;

	for(i = 0; i < 7; i++) {
		
		if (strcmp(roomList[i].type, "1ST_ROOM") == 0){
			return i;
		}

	}

	printf("Could not find Room with attribute '1ST_ROOM'"); //couldn't find it, buildrooms is broken
	perror("Error in getFirstRoomIndex()");
	exit(1);
}

/*
 * simple function to find which room is END_ROOM
 * prereqs: roomsList populated
 * returns: index of END_ROOM
 */
int getEndRoomIndex(){

	int i;

	for(i = 0; i < 7; i++) {
		
		if (strcmp(roomList[i].type, "END_ROOM") == 0){
			return i;
		}

	}

	printf("Could not find Room with attribute 'END_ROOM'"); //couldn't find it, buildrooms is broken
	perror("Error in getEndRoomIndex()");
	exit(1);
}

/*
* function to get the index of the selected room.
* prereqs: current room set
* returns: int index of next room [0..7]; 99 if not in list
*/
int getNextRoomIndex(int currRm, char *str){

	int i,j;
	int c = roomList[currRm].numCon;

	for(i = 0; i < c; i++) {																		//itr through current room's connections
		if (strcmp(roomList[currRm].connections[i], str) == 0){		//if room is in the connections list
			for(j = 0; j < 7; j++) {																//find the index in roomsList
				if (strcmp(roomList[j].name, str) == 0){
					return j;																						//return that index
				}
			}
		}
	}
	return 99;																									//not in connections list of current rm
}

/*
 * simple func to print required data about current room
 * prereqs: nameList populated
 * returns: none (writes to stdout)
 */
void printRoom(int index){

	int i;
	int c = roomList[index].numCon;	

	printf("CURRENT LOCATION: %s\n", roomList[index].name);
	printf("POSSIBLE CONNECTIONS: ");

	for (i = 0; i < c; i++) {
		printf("%s", roomList[index].connections[i]);
		if (i != c - 1) {
			printf(", ");
		} else {
			printf(".\n");
		}
	}

	return;
}

/*
 * function to generate the current time and date and write to file
 * prereqs: write ability in cwd, disk space (trivial)
 * returns: none (writes to file, sets global var timeFile)
 */
void* genTime(void * arg){

	time_t 	result;
	struct 	tm *currTime;
	int			len = 150;
	char		buf[len];
	//char		file[38];
	int			file_descriptor;

	memset(timeFile, 0, sizeof(timeFile));											//zero out file name

	pthread_mutex_lock(&myMutex);																//i have the squirrel

	result = time(NULL);																				//get the time (syscall)
	currTime = localtime(&result);															//mutate time to prep for strftime
	
	//get date in format ' 1:03pm, Tuesday, September 13, 2016'
	strftime(buf, len, " %l:%M%P, %A, %B %d, %Y\n", currTime);  //format time per reqs (yeah, that space is there in the example)
	sprintf( timeFile, "%s/currentTime.txt", dirName);					//generate complete file name
	file_descriptor = open(timeFile, O_WRONLY | O_TRUNC | O_CREAT, 0600);//open the file (truncate if exists, create if not)

	if (file_descriptor < 0) {
		printf("Could not open %s\n", timeFile);									//print errors
		perror("Error in getTime()");
		exit(1);																									//die
	}
	write(file_descriptor, buf, strlen(buf) * sizeof(char));		//write the file

	pthread_mutex_unlock(&myMutex);															//release the squirrel
	
	return;
}

/*
 * function to read time from file
 * prereqs: currentTime.txt created and populated
 * returns: none (prints time)
 */
void printTime(){

	int			file_descriptor;
	char		readBuf[170];															//file read container

	memset(readBuf, 0, sizeof(readBuf));							//clear readBuf

	file_descriptor = open(timeFile, O_RDONLY);				//open file
	//if the file doesn't open, catch it
	if( read(file_descriptor, readBuf, sizeof(readBuf)) == -1) {
		printf("Error reading file '%s'\n", timeFile);	//print error
		perror("Error in readFiles()");
		exit(1);																				//die
	}
	//print time
	printf("\n%s", readBuf);

	return;
}
