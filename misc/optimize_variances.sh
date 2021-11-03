for file in ../data/*.binpsllh; do
size=$(stat --printf="%s" "$file")
n=$(((size - 8) / 8))
variance=$(./tree_simulation $n 256 0.0 | grep -F "Best optimization with variance" | cut -d' ' -f5)
echo $(basename "$file"),$n,$variance
done
