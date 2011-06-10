#
# Make sure to turn on the BITS 32 directive in the input file
#
nasm -f bin -O2 $1 -o $2
