all: binary_tree

binary_tree: binary_tree.cpp
	mpic++ binary_tree.cpp io.cpp -o binary_tree -std=c++20 -Wall -O3

binary_tree_debug: binary_tree.cpp
	mpic++ binary_tree.cpp io.cpp -o binary_tree_debug -std=c++20 -Wall -ggdb

binary_tree_sim: binary_tree.cpp
	smpicxx binary_tree.cpp io.cpp -o binary_tree_sim -std=c++20 -Wall -O3

simgrid: binary_tree_sim
	smpirun -np 1 ./binary_tree_sim data/XiD4.psllh

clean:
	rm -f binary_tree{,_sim,_debug}


