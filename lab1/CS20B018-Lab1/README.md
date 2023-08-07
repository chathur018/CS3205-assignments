run make all to create the executables (tested to work on Linux)

1. 
run server with:
./UDPechod [port]

run client with
./UDPecho localhost [port]

The client will request a input string. The given string will be sent to the server.
The server will echo back the string. The client prints out the returned string.
The client can send a maximum of 127 chars in one message.


2.
run server with:
./UDPmathd [port]

run client with
./UDPmath localhost [port]

The client will request a command. The given string will be sent to the server.
The server will return the result as a string. The client prints out the returned string.
If the operator is not recognized by the server, it returns an error message.
For the hyp operator, the result is truncated down to an integer.


3.
run server with:
./TCPfiled [port]

run client with
./TCPfile localhost [port]

Provide the client with a filename followed by the number of bytes required.
The server returns the number of bytes requested from the end the file.
If the file does not exist on the server, it returns a speacial message.
If recieved the client will print out the message, create a new file with name filename1 and save the message to it.
