#!/usr/bin/python
# ftclient.py
# programmed by: Rick Menzel
# for: CS 372, Winter 2018

import socket
import sys

COMM_BUF_SIZE = 128
FILE_BUF_SIZE = 104857600

# simple helper function to print out usage instructions
# prereqs: none
# returns: none
def usage():
	print "USAGE: %s <server_hostname> <server_port> <command> <data_port> <file_name>" % sys.argv[0]

def invalid_command():
	print "Valid commands are:\n\tlist: -l <data_port>\n\tget:  -g <data_port> <file_name>"

# function to turn command line arguments into useful, globally accesible variables
# prereqs: imported sys
# returns: none
def parse_args():
	global ARG_COUNT																						# declare globals
	global FILE_NAME
	global DATA_PORT
	global SERVER_HOST
	global SERVER_PORT
	global COMMAND

	ARG_COUNT = len(sys.argv)																		# get the number of args for this call
	FILE_NAME = "NULLFILE"
	if ARG_COUNT < 5 or ARG_COUNT > 6:													# check for valid arg count (list = 5 args, get = 6 args)
		usage()																										# show usage on bad call
		invalid_command()																					# show command list
		exit(1)																										# die on invalid call
	elif ARG_COUNT is 6:																				# set the 2 extra parameters if we are requesting a file
		FILE_NAME = sys.argv[5]																			# NOTE: see usage() for list/order of proper arguments
	DATA_PORT = sys.argv[4]																			# set the basic parameters for all valid calls
	SERVER_HOST = socket.gethostbyname(sys.argv[1])							
	SERVER_PORT = int(sys.argv[2])
	COMMAND 		= sys.argv[3]
	if COMMAND != "-l" and COMMAND != "-g":											# validate command. this should be changed to accomodate longer command lists
		invalid_command()																					# show command options and die on bad call
		exit(1)

# function to setup the server end of a data connection and read data from that connection
# prereqs: post is a valid port number
# returns: a string containing recvd data
def handle_data_connection(port):
	HOST = ''																									# using "" will allow either the actual name (printed below) or localhost if on same machine
	PORT = int(port)																					# use passed in data port number

	print "ftclient: initializing data connection... hostname is %s on port %d" % (socket.gethostname(), PORT)	# display status with hostname and port number
	dataSock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)	# setup socket
	dataSock.bind((HOST, PORT))																# bind socket																		
	print "ftclient: waiting for data connection..."					# display status
	dataSock.listen(1)																				# listen for incoming connections

	dataConn, dataAddr = dataSock.accept()										# accept inc connection
	print "ftclient: data connection established with", SERVER_HOST
	total_data = []																						# initialize container
	while True:																								# keep reading as long as there is data on the line
		data = dataConn.recv(256)																# get the next chunk
		if not data: break																			# no data = end of transmission
		total_data.append(data)																	# add the data to our string

	dataConn.close()																					# close the data connection
	print "ftclient: data connection closed with", dataAddr		# verbose
	return ''.join(total_data)																# return the data list as a string

# simple helper to write data string to a file
# prereqs: data is a valid string
# returns: none
def write_file(data):
	try:																											# try to open file as read-only (check if it exists)							
		fptr = open(FILE_NAME, "r")
	except IOError:																						# file doesn't exist
		print "Writing file..."																	# verbose
		fptr = open(FILE_NAME, "w")															# open the file
		fptr.write(data)																				# write data
		fptr.close()																						# close the file
	else:																											# file does exist
		confirm = raw_input( "ftclient: WARNING! File already exists. Do you wish to overwrite? ")	# prompt for overwrite y/n
		if confirm.lower() == "yes" or confirm.lower() == "y":	# check user response
			print "ftclient: Writing file..."											# overwrite on yes
 			fptr = open(FILE_NAME, "w")														# open the file 
			fptr.write(data)																			# write data		
			fptr.close()																					# close the file
		else:																										# no overwrite
			print "ftclient: Skipping file write. Please try again with a different file."
	
# begin main program
parse_args()																								# parse command line args
controlConn = socket.socket(socket.AF_INET, socket.SOCK_STREAM) #set up control connection
controlConn.connect((SERVER_HOST, SERVER_PORT))							# connect to server

if ARG_COUNT is 5:																					# assemble command msg; will include all data either way (configurable)
	commandBuffer = COMMAND + " " + DATA_PORT + " " + FILE_NAME + "\0" * (COMM_BUF_SIZE - len(COMMAND) - len(FILE_NAME) - len(DATA_PORT) - 2)
elif ARG_COUNT is 6:
	commandBuffer = COMMAND + " " + DATA_PORT + " " + FILE_NAME + "\0" * (COMM_BUF_SIZE - len(COMMAND) - len(FILE_NAME) - len(DATA_PORT) - 2)

controlConn.sendall(commandBuffer)													# send the command msg
data = handle_data_connection(DATA_PORT)										# set up data connection and receive data

if COMMAND == "-l" and data != "ERROR":											# valid list command
	print "server:", data																			# print to terminal
elif COMMAND == "-g" and data != "ERROR":										# valid get command
	write_file(data) 																					# handle file writing
	print "ftclient: transfer complete"												# per program specs
else:																												# invalid or malformed command
	data = controlConn.recv(COMM_BUF_SIZE)										# pull error msg from server
	print data
controlConn.close()																					# close control connection
print "ftclient: connection closed with", SERVER_HOST				# verbose
