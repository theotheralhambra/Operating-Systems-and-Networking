#!/usr/bin/python
# chatserver.py
# programmed by: Rick Menzel
# for: CS 372, Winter 2018

import socket
import sys

# simple helper function to print out usage instructions
# prereqs: none
# returns: none
def usage():
	print "USAGE: %s <port number>" % sys.argv[0]

# performs setup (request handling/acknowledgement) of connection
#	prereqs: conn, addr initialized with valid values
# returns: 1 for go, 0 for no-go
def setup_connection(conn, addr):
	ACK_MSG = "ACKPROC"
	data = conn.recv(7) 																			# get conn request
	if not data:																							# make sure we got something
		return 0
	if data != "PORTNUM":																			# verify that connection request is valid
		return 0
	conn.sendall(ACK_MSG)																			# send acknowledgement
	print "Connected with client at", addr										# display status and instructions
	print "Enter \quit to quit chatting.\n"
	return 1	

# simple helper funtion to reader user input from terminal
# prereqs: handle has a valid string value
# returns: use rinput test as str
def prompt(handle):
	msg = raw_input("%s> " % handle)													# prompt and read input
	return msg
	
# begin main program
if len(sys.argv) != 2 :																			# check for proper arg count
	usage()																										# show usage on bad call
	exit(1)
else:
	HOST = ""																									# using "" will allow either the actual name (printed below) or localhost if on same machine
	PORT = int(sys.argv[1])																		# cast command line arg to int
	HANDLE = "server"																					# hard coded per program reqs
	print "server initializing..."														# display status with hostname and port number
	print "hostname: %s on port %d" % (socket.gethostname(), PORT)
	sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)	# setup socket
	sock.bind((HOST, PORT))																		# bind socket
	# begin server listening loop																
	while( 1 ): 																							
		print "waiting for connection..."												# display status
		sock.listen(1)																					# listen for incoming connections
		conn, addr = sock.accept()															# accept inc connection
		if not setup_connection(conn, addr):										# handle comm request from client
			conn.close()																					# for bad request, close connection and revert to listening
			continue
		# begin comm loop																				
		while 1:																								
			comm_buffer = conn.recv(512)													# recv 512 bytes from socket (per program req's: 10 for handle + 2 format + 500 msg text)
			if not comm_buffer:																		# connection is broken, close and revert to listening 
				break
			if "\\quit" in comm_buffer:														# test for quit notification
				print "Connection terminated by client."
				break
			sys.stdout.write(comm_buffer)													# display the recv'd msg (includes sender handle). using write() avoids extra \n
			sys.stdout.flush()																		# flush stdout, req after write()
			input_buffer = prompt(HANDLE)													# get user response
			if input_buffer == "\\quit":													# check for quit request
				conn.sendall( "\quit" )															# if found, send ONLY quit command to client
				break
			comm_buffer = HANDLE + "> " + input_buffer						# assemble outgoing msg (format: handle + "> " + msg_text)
			conn.sendall( comm_buffer )														# send assembled msg
		conn.close()																						# end comm loop, close connection, revert to listening
		print "Connection closed with", addr
