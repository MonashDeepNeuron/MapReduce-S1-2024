mpic++ -Wall -o testMPI source/hello.cpp
mpirun -np $1 ./testMPI
