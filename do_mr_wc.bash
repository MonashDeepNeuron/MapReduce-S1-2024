#!/bin/bash

nthreads=$1
files="data/pg11.txt data/pg76.txt data/pg84.txt data/pg98.txt data/pg100.txt data/pg145.txt data/pg174.txt data/pg394.txt data/pg1260.txt data/pg1342.txt data/pg1513.txt data/pg2542.txt data/pg2641.txt data/pg2701.txt data/pg5200.txt data/pg6761.txt data/pg16389.txt data/pg25717.txt data/pg28054.txt data/pg37106.txt data/pg52106.txt data/pg67979.txt data/pg73622.txt"
./mr_wc $nthreads $files
