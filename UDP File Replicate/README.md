README - The current file being read
Makefile - A file that contains all components to create the executable for the program myclient and myserver
myclient.c - A program that simulates a file transfer using socket programming with UDP so that files can be sent to the server in a reliable manner. It makes use of threading because this program allows for multiple server to be open so that the client can send the file to all of them concurrently. Once
the client reads a file based and sends all of the packets to the server, then it can reassemble it
to recreate the file under a new file name.
myserver.c - A program that simulates a server that the client can send information to and receive back the ACKs to make sure the data was received in a proper manner. It also writes the sent data into a 
new file given by the user and recreates it on the server side.
Docu.txt - A document that lists 5 tests performed on the program for functionality and briefly
describes how the program works.
