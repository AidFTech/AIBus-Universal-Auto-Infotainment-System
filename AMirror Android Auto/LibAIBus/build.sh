for c in *.cpp; do
	input="$c"
	output=${input//".cpp"/}
	
	output=$(printf "%s.o" "$output")
	
	g++ -c $input -o $output
done

ar rcs -o libaibusclient.a ./*.o

rm ./*.o
