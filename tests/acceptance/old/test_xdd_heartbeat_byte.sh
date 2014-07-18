#!/bin/bash
#
# Description - outputs current amount of Bytes, Kilobytes, Megabytes, and Gigabytes 
#
# Verify -hb byte, kbyte, mbyte, gbyte by checking if heartbeat output bytes equals actual transfered bytes and if output bytes matches output kbytes, mbytes, and gbytes
#
# Source the test configuration environment
#
source ./test_config

# Pre-test set-up

test_name=$(basename $0)
test_name="${test_name%.*}"
test_dir=$XDDTEST_LOCAL_MOUNT/$test_name
mkdir -p $test_dir
test_file=$test_dir/data1


os=`uname`

xdd_byte_cmd="-hb bytes -hb kbytes -hb mbytes -hb gbytes"

times_test=0

# rerun the test if xdd_getfilesize pulls 3 or less sizes from the target file
while [ $times_test -le 7 ]; do  

    rm $test_dir/*
      
    echo "cookies"  
    sleep 1

    $XDDTEST_XDD_EXE -target $test_file -op write -runtime 15 -reqsize 7000  -numreqs 7000 $xdd_byte_cmd -hb lf -hb output $test_dir/data2 &

    while [ ! -e $test_file ]; do
        echo "asdf"
    done
     

    num_ele=0
    process=1
    if [ "$os" == "Darwin" ]; then
        while [ -n "$process" ]; do
            sleep 0.9          # sleep 1 to sync with xdd since hb outputs every second
            num_ele=$(($num_ele+1))
            process=$(ps | grep $test_file | grep -v grep)
            xdd_get_size[$num_ele]=$($XDDTEST_XDD_GETFILESIZE_EXE $test_file | cut -f 1 -d ' ')
        done

    # Linux gets the process differently
    elif [ "$os" == "Linux" ]; then
        while [ -n "$process" ]; do
            sleep 0.9          # sleep 1 to sync with xdd since hb outputs every second
            num_ele=$(($num_ele+1))
            process=$(ps -e| grep -w "xdd")
            xdd_get_size[$num_ele]=$($XDDTEST_XDD_GETFILESIZE_EXE $test_file)
        done

    fi

    echo "xdd_get_file elements : $num_ele" 


    # copy data2.T0000.csv to data3 without the empty first line
    sed 1d $test_dir/data2.T0000.csv > $test_dir/data3


    line_count=$(wc $test_dir/data3)
    line_count=$(echo $line_count | cut -f 1 -d ' ')

    # determine whether the array containing data from xdd_getfilesize or heartbeat has more elements
    if [ $num_ele -le $line_count ]; then
        times_test=$num_ele
    else
        times_test=$line_count
    fi


    echo "num_ele $num_ele lines $line_count"
    echo "used: $times_test"


    for i in {1..12}; do
        echo ${xdd_get_size[$i]} $i
    done

    declare -a byte
    declare -a kbyte
    declare -a mbyte
    declare -a gbyte

    for i in $(seq 1 $line_count); do
        byte=$(sed -n "$i"p $test_dir/data3 | cut -f 4 -d ',')  # remove trailing 0s
        byte[$i]=$(echo $byte | sed 's/^0*//')
        
        kbyte=$(sed -n "$i"p $test_dir/data3 | cut -f 6 -d ',') # remove trailing 0s and truncate
        kbyte[$i]=$(echo ${kbyte%.*} | sed 's/^0*//')

        mbyte=$(sed -n "$i"p $test_dir/data3 | cut -f 8 -d ',') # remove trailing 0s and truncate 
        mbyte[$i]=$(echo ${mbyte%.*} | sed 's/^0*//')

        gbyte=$(sed -n "$i"p $test_dir/data3 | cut -f 10 -d ',') # multiply gbytes by 10 and remove
        gbyte=$(echo "$gbyte*10" | bc)                           # decimal to get rid floating points  
        gbyte[$i]=$(echo ${gbyte%.*})

        echo "B:${byte[$i]} XDD:${xdd_get_size[$i]} K:${kbyte[$i]} M:${mbyte[$i]} G:${gbyte[$i]}"
    done

    # find the percent error in actual vs approx file size
    sum_errors=0
    error_bound=20

    outlier_count=0
    outlier_bound=$(($times_test/3))     # cannot have more than a third of the results be outliers 

    for i in $(seq 1 $times_test); do
        error=$((${byte[$i]}-${xdd_get_size[$i]}))          
        abs_error=$(echo ${error#-})
        rel_error=$(($error*100))
        rel_error=$(($rel_error/${xdd_get_size[$i]}))       
        rel_error=$(echo ${rel_error#-})     # remove negative sign

        sum_errors=$(($sum_errors+abs_error))
        

        echo "cakes : $rel_error from_hb: ${byte[$i]} from_xdd: ${xdd_get_size[$i]} hb-xdd: $abs_error"

        if [ $rel_error -ge $error_bound ]; then
            outlier_count=$(($outlier_count+1))
        fi 
    done

    avg_error=$(($sum_errors/$times_test))  

    pass_count=0
    error_bound=100000000
    # error bound for mac os x should be 50000000    


    if [ $avg_error -le $error_bound ]; then
        pass_count=1
    fi

    echo "brownies $avg_error $error_bound"

done #while times_test

      echo "fudge outlier count: $outlier_count pass count: $pass_count"

      if [ $outlier_count -le $outlier_bound -a $pass_count -eq 1 ]; then
        echo "passed"
        exit 0
      fi

      exit 1

