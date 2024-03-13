#!/bin/bash
mkdir test
cp -r -t test code
cd test/code
cat ../../code/14.c
gcc -c *.c
gcc -o hello *.o
./hello 2>err.txt
mv -t ../../ err.txt
cd ../../
chmod 655 err.txt
if (( $# == 2 )); then
	n=$(($1+$2))
elif (( $# == 1)); then
	n=$(($1+1))
else
	n=2
fi
sed -n "${n}p" err.txt>&2

