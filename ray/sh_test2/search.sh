#!/bin/bash
#First you can use grep (-n) to find the number of lines of string.
#Then you can use awk to separate the answer.
file=$1
str=$2
result=$3

grep -n $str $file | awk -F: '{print $1}' > $result
