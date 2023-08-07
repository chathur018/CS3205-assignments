#!/bin/bash

make clean
make

echo "Enter number of routers: "
read num

echo "Enter name of input file: "
read input

echo $num

for (( i=0 ; i<$num ; i++ ))
do
    gnome-terminal -e "./ospf -i $i -f $input -o outfile-$i.txt -h 1 -a 2 -s 5"
done
