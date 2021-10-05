all: binomial_broadcast binomial_reduce binary_tree

binomial_broadcast: binomial_broadcast.cpp
	mpic++ binomial_broadcast.cpp -o binomial_broadcast -ggdb -std=c++20 -Wall

binomial_reduce: binomial_reduce.cpp
	mpic++ binomial_reduce.cpp -o binomial_reduce -ggdb -std=c++20 -Wall

binary_tree: binary_tree.cpp
	mpic++ binary_tree.cpp io.cpp -o binary_tree -ggdb -std=c++20 -Wall

clean:
	rm binomial_broadcast binary_tree
