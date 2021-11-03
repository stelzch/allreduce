for file in ../data/*.binpsllh; do
size=$(stat --printf="%s" "$file")
n=$(((size - 8) / 8))
echo Testing $file with $n summands
done
