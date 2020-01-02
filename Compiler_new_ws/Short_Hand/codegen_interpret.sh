#!/bin/sh
if [ $# -ne 2 ]
then
echo "This script needs two argument which is the input file and option"
echo "Usage ./run.sh input_file"
exit 0
fi
echo "Compiling..."
make -j4 # Compile the code
echo "Producing the output LLVM IR Code"
./build/short_hand $1 $2

echo "Generating Bitecode from IR"
llvm-as ir_file.ir -o bytecode_file.bc

echo "Interpreting Bitecode"
lli bytecode_file.bc
