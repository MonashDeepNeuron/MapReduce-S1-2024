# MapReduce-S1-2024
MapReduce-S1-2024
# What is Map-Reduce?
MapReduce is a framework for designing parallel computations in a way that can be distributed over warehouse-scale computers, popularised by a [Google paper](https://static.googleusercontent.com/media/research.google.com/en//archive/mapreduce-osdi04.pdf). Within this framework, a client provides the data in batches, a Map function to split files of the data into {key:value} pairs, and a Reduce function that describes how to combine the values of elements with the same key. An essential property of the Map and Reduce operations is that they are associative (can be executed in any sequence) which allows the computation to be distributed over many worker units.

# Dependencies
- [OpenMPI](https://docs.open-mpi.org/en/v5.0.x/index.html)

# Build and Run
Use the provided `build.sh` hopefully.
To run setup openmpi, I've written `openmpi.sh` for sourcing openmpi and loading the module on my local Fedora linux install
- Use `source openmpi.sh` to load openmpi module on Fedora linux (after installing it)
