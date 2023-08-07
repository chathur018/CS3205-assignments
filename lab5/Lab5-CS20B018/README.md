To clean the directory, run:
make clean

To compile, run:
make

To run a router, use:
./ospf -i [id] -f [iput-file] -o [output-file] -h [hello-interval] -a [lsa-interval] -s [spf-interval]

Run the above command for all the ids in your input file

There is also a test script which will automatically compile and launch all the processes. Run:
./test.sh
Provide the number of routers and the input file
Each router is launched in its own terminal.

The submission contains the two inputs which were used for the report and the output files produced by the first input file.