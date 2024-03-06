#!/bin/bash
input=$1
old_str=$2
new_str=$3

sed -i "s/$old_str/$new_str/g" $input
