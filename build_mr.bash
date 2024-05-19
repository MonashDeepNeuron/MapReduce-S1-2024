#!/bin/bash

g++ -fopenmp -Wall -O3 -o build/mr_wc source/omp_mapreduce.cpp
