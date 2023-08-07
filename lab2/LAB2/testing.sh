#!/bin/bash

make clean

make

./lab2-cs20b018 20000 lab2-input1-localservers.txt < sample-queries.txt
