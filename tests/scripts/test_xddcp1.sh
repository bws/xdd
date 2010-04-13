#!/bin/bash

###########################################################
#
# - test_xddcp1.sh -
#
# Tests the performance of xddcp between two machines and
# saves the results as comma separated values (.csv) file
#
# To use, configure the options below,
# then run this script on the source machine
#
###########################################################

# csv file to write out
csv_file="results.csv"
# file to read
source_file="/data/xfs/8gb.dat"
# host where we send the file
destination='192.168.1.4'
# file to write on the destination host
destination_file="/data/xfs/8gb.dat"
# size of the file to transfer,
# leave blank to use the size of source_file
file_size=""
# absolute location of source log files
# WARNING: This needs to be an empty directory
source_logs="/dev/shm/xdd"
# absolute location of destination log files
# WARNING: This needs to be an empty directory
destination_logs="/dev/shm/xdd"
# set auto_delete_logs to 1 if you want all of the
# log files to be removed at the end of the script
auto_delete_logs=1
# number of threads
threads=4
# number of runs
runs=10
# xddcp command to use
xddcp='xddcp'
# xddcp options (use '-d -s' to enable DIO on both sides)
options=''

###########################################################
# End of Options
###########################################################


# functions used for generating the csv file
function parse_src_logs {
	for log in $source_logs/*.log; do
		echo -n "$log,$threads,"
		echo -n `basename "$log" .log | sed -e "s/.*\.//g"`
		grep " COMBINED" $log | head -n1 | sed -e "s/\r /,/g" | sed -re "s/[ \t]+/,/g"
	done
}
function parse_dst_logs {
	(ssh $destination bash <<EOF
	for log in $destination_logs/*.log; do
		echo -n "\$log,$threads,"
		echo -n \$(basename "\$log" .log | sed -e "s/.*\.//g")
		grep " COMBINED" \$log | head -n1 | sed -e "s/\r /,/g" | sed -re "s/[ \t]+/,/g"
	done
EOF
	) 2> /dev/null
}
function make_csv {
	echo "timestamp,threads,filesize,src_seconds,src_mbps,dst_seconds,dst_mbps" > $csv_file
	parse_src_logs | cut -d , -f 2,3,8,10,11 > tmpsrc.csv
	parse_dst_logs | cut -d , -f 3,10,11 > tmpdst.csv
	join -t , -1 2 -2 1 tmpsrc.csv tmpdst.csv >> $csv_file
	rm tmpsrc.csv tmpdst.csv
}

# make sure the log folders are empty
if [ "$(ls -A $source_logs)" ]; then
	echo "$source_logs on $HOSTNAME is not empty!"
	echo "Exiting..."
	exit
fi
if [ "$((ssh $destination ls -A $destination_logs) 2> /dev/null)" ]; then
	echo "$destination_logs on $destination is not empty!"
	echo "Exiting..."
	exit
fi

# print basic info

echo "---------------------------------------------------------------------"
echo "source filename: $source_file"
echo "destination host: $destination"
echo "destination filename: $destination_file"
if [ -z "$file_size" ]; then
	echo "file size: `stat -c %s $source_file`"
else
	echo "file size: $file_size"
fi
echo "source log folder: $source_logs"
echo "destination log folder: $destination_logs"
echo "number of threads: $threads"
echo "number of runs: $runs"
echo "---------------------------------------------------------------------"

# main loop
echo "Entering main loop... "
for i in `seq 1 $runs`; do
	($xddcp $options -t $threads -S $source_logs -D $destination_logs \
		$source_file $destination:$destination_file $file_size) &> /dev/null
	echo "Run $i completed `date`."
done
echo "Done."

# parse logs and make csv files
echo -n "Parsing log files... "
make_csv
echo "Done."

# clean out log folders if needed
if [ $auto_delete_logs = 1 ]; then
	echo -n "Deleting log files... "
	rm $source_logs/*.log
	(ssh $destination rm $destination_logs/*.log) &> /dev/null
	echo "Done."
fi



