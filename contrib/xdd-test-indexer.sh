#!/bin/bash
#
#
#
# create arrays to store description and name
declare -a file_head
declare -a file_name
count=0

# where the tests are at
test_dir=~/workspace/xdd/tests/acceptance
test_file=$test_dir/test_xdd*


# store description and name into arrays
for file in $test_file; do
    file_head[$count]=$(head -n 15 $file | grep Description)  
    file_name[$count]=$file
    count=$(($count+1))
done

# print out tests and descriptions
for i in i$(seq 1 $count); do
    temp_file=${file_name[$i]}
    echo ${temp_file##*/}
    echo $'\t' ${file_head[$i]} $'\n'
done
