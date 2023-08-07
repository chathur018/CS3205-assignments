To clean the directory run:
make clean

To compile run:
make

To run the code run the following two instructions on seperate shells:

Client: ./RecieverGBN -p 12345 -n 400 -e 0.1 -d

Server: ./SenderGBN -p 12345 -l 512 -r 10 -n 400 -w 3 -b 10 -d

Other flags mentioned in the assignment may also be used.
-d flag can be omitted to run in non-debug mode.