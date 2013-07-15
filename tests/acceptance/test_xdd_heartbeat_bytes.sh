#!/bin/bash
#
# Description - outputs current amount of (k, m, g)bytes 
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

# run xdd while kb doesn't end in .5 to avoid roundoff errors
while [ $passing -eq 0 ];do
    passing=1
    rm $test_dir/*

    test_file=$test_dir/data1
    touch $test_file
    xdd_byte_cmd="-hb bytes -hb kbytes -hb mbytes -hb gbytes"
    $XDDTEST_XDD_EXE -target $test_file -op write -reqsize 650  -numreqs 650 -hb lf $xdd_byte_cmd -hb output $test_dir/data2 &
  
    for i in {1..6};do
        sleep 1
        file_size[$i]=$(stat $test_dir/data1 | cut -f 9 -d ' ')
    done
    
    wait

    line=$(wc $test_dir/data2.T0000.csv)
    line_count=$(echo $line | cut -f 1 -d ' ')
     
    for ((i=1; i<=$line_count; i++)); do
        ending=$(grep "Pass" $test_dir/data2.T0000.csv | cut -c 78 | sed -n "$i"p)
        if [ $ending -eq 5 ]; then
            passing=0
            break
        fi
        
    done

     # store bytes, kilbytes, etc.
    for byte_num in {1..6}; do
        byte=$(cut -f 4 -d ',' $test_dir/data2.T0000.csv | tail -$line_count | sed -n "$byte_num"p)
        byte[$byte_num]=$(echo $byte | sed 's/^0*//')

        kbyte=$(cut -f 6 -d ',' $test_dir/data2.T0000.csv | tail -$line_count | sed -n "$byte_num"p)
        kbyte=$(echo $kbyte | sed 's/^0*//')
        kbyte[$byte_num]=$(printf "%1.f" "$kbyte")
        
        mbyte=$(cut -f 8 -d ',' $test_dir/data2.T0000.csv | tail -$line_count | sed -n "$byte_num"p)
        mbyte=$(echo $mbyte | sed 's/^0*//')
        mbyte=$(echo "$mbyte+.001" | bc)
        mbyte[$byte_num]=$(printf "%1.f" "$mbyte")
        
        gbyte[$byte_num]=$(cut -f 10 -d ',' $test_dir/data2.T0000.csv | tail -$line_count | sed -n "$byte_num"p)
    done
done

declare -a correct
error_bound=10

# See if outputed current bytes equals actual transfered bytes
for i in {1..6}; do
    correct[$i]=0
    abs_error=$((${byte[$i]}-${file_size[$i]}))  
    abs_error=$(echo ${abs_error#-}) 

    rel_error=$(echo "scale=2;$abs_error/${byte[$i]}" | bc)
    rel_error=$(echo "$rel_error*100" | bc)
    rel_error=$(echo ${rel_error%.*})
    
    if [ $rel_error -lt $error_bound ]; then
        correct[$i]=1
    fi
done


# See if bytes matches kbytes, mbytes, and gbytes
for i in {1..6}; do 
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
                match[$i]=1
            fi
        fi
     fi
done

# adds 1 to arrays sum and sum2 if matching and 0 if not
sum=0
sum2=0
for i in {1..6}; do
    sum=$[$sum+${match[$i]}]
    sum2=$[$sum2+${correct[$i]}]
done

# if all match then sum and sum2 should equal 6 
test_success=0
if [ $sum -eq 6 -a $sum2 -eq 6 ]; then
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

