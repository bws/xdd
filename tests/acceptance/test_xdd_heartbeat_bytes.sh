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

passing=0
declare -a match
declare -a file_size
declare -a byte
declare -a kbyte
declare -a mbyte
declare -a gbyte

# run a pass of xdd while heartbeat output Kilobyte doesn't end in .5 to avoid roundoff errors during conversions
while [ $passing -eq 0 ];do
    passing=1

    rm $test_dir/*

    test_file=$test_dir/data1
    
    xdd_byte_cmd="-hb bytes -hb kbytes -hb mbytes -hb gbytes"
    $XDDTEST_XDD_EXE -target $test_file -op write -runtime 6 -reqsize 1000  -numreqs 1000 $xdd_byte_cmd -hb lf -hb output $test_dir/data2 &

    process=1
    i=1
    while [ -n "$process" ]; do
        process=$(ps | grep $test_file | grep -v grep)
        sleep 1          # sleep 1 to sync with xdd since hb outputs every second
        file_size[$i]=$($XDDTEST_XDD_GETFILESIZE_EXE $test_file | cut -f 1 -d ' ')
        i=$(($i+1))
    done

    wait

    line_count=$(wc $test_dir/data2.T0000.csv)
    line_count=$(echo $line_count | cut -f 1 -d ' ')
    
    # test if number of Kilobytes ends in 5 (chararcter 78 in output file)
    for ((i=1; i<=$line_count; i++)); do 
        ending=$(grep "Pass" $test_dir/data2.T0000.csv | cut -c 78 | sed -n "$i"p)
        if [ $ending -eq 5 ]; then
            passing=0    # Rerun test if Kilobytes ends in 5 to avoid rounding error 
            break
        fi
        
    done


    # since the first line of the heartbeat output file is empty, two counters are used, 
    # one that starts on 1 (index) for indexing the array and one that starts on 2 (i) for 
    # skipping the empty line
    index=1

    # store Bytes, Kilobytes, Megabytes, and Gigabytes into arrays
    for i in $(seq 2 $(($line_count+1))); do
        byte=$(cut -f 4 -d ',' $test_dir/data2.T0000.csv | sed -n "$i"p)
        byte[$index]=$(echo $byte | sed 's/^0*//')

        kbyte=$(cut -f 6 -d ',' $test_dir/data2.T0000.csv | sed -n "$i"p)
        kbyte=$(echo $kbyte | sed 's/^0*//')
        kbyte[$index]=$(printf "%1.f" "$kbyte")
        
        mbyte=$(cut -f 8 -d ',' $test_dir/data2.T0000.csv | sed -n "$i"p)
        mbyte=$(echo $mbyte | sed 's/^0*//')
        mbyte=$(echo "$mbyte+.001" | bc)
        mbyte[$index]=$(printf "%1.f" "$mbyte")
        
        gbyte[$index]=$(cut -f 10 -d ',' $test_dir/data2.T0000.csv  | sed -n "$i"p)
        index=$(($index+1))
    done

done

error_bound=15    
pass_count=0

# See if current bytes equals actual transfered bytes
for i in $(seq 1 $line_count); do
    error=$((${byte[$i]}-${file_size[$i]}))  
    abs_error=$(echo ${error#-}) 

    rel_error=$(echo "scale=2;$abs_error/${byte[$i]}" | bc)
    rel_error=$(echo "$rel_error*100" | bc)
    rel_error=$(echo ${rel_error%.*})
    echo "$rel_error"
    # Test if heartbeat's current bytes is within the error_bound for actual bytes
    if [ $rel_error -lt $error_bound ]; then
        pass_count=$((pass_count+1))
    fi
done


if [ $pass_count -eq $line_count ]; then
    # No errors when comparing heartbeat bytes to actual 
    # Now verify that bytes matches kbytes, mbytes, and gbytes
    
    for i in $(seq 1 $line_count); do 
        conv_b=$(echo "scale=2;${byte[$i]}/1024" | bc) 
        
        conv_k=$(echo "scale=2;${kbyte[$i]}/1024" | bc) 
        conv_k=$(echo "$conv_k+.001" | bc)
        
        conv_m=$(echo "scale=2;${mbyte[$i]}/1024" | bc) 
        conv_m=$(echo "$conv_m+.001" | bc)
        
        round_b=$(printf "%1.f" "$conv_b")
        round_k=$(printf "%1.f" "$conv_k")
        round_m=$(printf "%.1f" "$conv_m")

        comp_b=$(echo $round_b'=='${kbyte[$i]} | bc -l)
        comp_k=$(echo $round_k'=='${mbyte[$i]} | bc -l)
        comp_m=$(echo $round_m'=='${gbyte[$i]} | bc -l)

        if [ $comp_b -eq 1 ]; then
            if [ $comp_k -eq 1 ]; then
                if [ $comp_m -eq 1 ]; then
                    pass_count=$(($pass_count+1))
                fi
            fi
         fi
    done
fi

# line_count is multiplied by 2 because pass_count is incremented by line_count while 
# comparing heartbeat output bytes to actual and incremented again by line_count when comparing
# bytes to kbytes to mbytes to gbytes
test_success=0
if [ $pass_count -eq $(($line_count*2)) ]; then    
    test_success=1
fi


# Verify output
echo -n "Acceptance Test - $test_name : "
if [ $test_success -eq 1 ]; then
    echo "PASSED"
    exit 0
else    
    echo "FAILED"   
    exit 1
fi 

