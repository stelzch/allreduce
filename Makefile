all: binomial_broadcast

binomial_broadcast: binomial_broadcast.cpp
	mpic++ binomial_broadcast.cpp -o binomial_broadcast -ggdb -std=c++20 -Wall
#binomial_broadcast.jpg: binomial_broadcast
#	mpirun -np 7 --mca opal_warn_on_missing_libcuda 0 ./binomial_broadcast
clean:
	rm binomial_broadcast
