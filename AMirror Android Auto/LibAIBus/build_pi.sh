for c in *.cpp; do
	input="$c"
	output=${input//".cpp"/}
	
	output=$(printf "%s.o" "$output")
	
	aarch64-linux-gnu-g++ --sysroot=/home/aidan/RPI_Sysroot -I /home/aidan/RPI_Sysroot/usr/local/include  -c $input -o $output
done

aarch64-linux-gnu-ar rcs -o libaibusclient.a ./*.o

rm ./*.o
