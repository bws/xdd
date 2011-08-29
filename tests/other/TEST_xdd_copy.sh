#!/bin/sh
#
# IMPORTANT NOTE: I found it necessary to delete the destination file
# before every test *IF* the same namespace is used AND the test used
# a smaller file size than the previous test. It seems as though the
# destination file is not truncated to the newer, SMALLER size, thus
# md5sum goes after the old size and gives different sums. I know WHAT
# happens, but not WHY. Is this an XDD or XFS issue??? Just be forewarned.
#


if [ "$1" == "-h" ]
then
echo "xddcp [ -d -s -t threads ] source_file destination_host destination_file [bytes_to_transfer]"
#echo "source_host       - source host IP or Name over which data is transferred"
echo "-d		- Use direct I/O on the destination end"
echo "-s		- Use direct I/O on the source end"
echo "-t threads	- Use 'threads' number of threads during transfer"
echo "source_file       - complete /filepath/name for source file on source host"
echo "destination_host  - destination host IP or Name over which data is transferred"
echo "destination_file  - complete /filepath/name for destination file on destination host"
echo "bytes_to_transfer - total bytes to transfer (default - size of source file)"
echo "Note: 'xdd' and script 'qipcrm' must be in your PATH env on both source and destination hosts!!!"
exit -1
fi

if [ $# -lt 2 ]
then
	echo "usage: $0 [ -d -s -t threads ] /full_path/source_file destination_host /full_path/destination_file [bytes_to_transfer]"
	exit -1
fi

which qipcrm > /dev/null 2> /dev/null

if [ $? -eq 1 ]; then
	echo "qipcrm executable was not found.  Please make sure it in your PATH."
	exit -1
fi

which xdd >/dev/null 2> /dev/null

if [ $? -eq 1 ]; then
        echo "xdd executable was not found.  Please make sure it in your PATH."
        exit 1
fi

uselocalDIO=" "		# To use Direct IO on the local side
useDIO=" "		# To use Direct IO on the remote side
queuedepth=4       # queuedepth = number of I/O threads

while getopts dst: option
do case $option in
	d) useDIO="-dio";;
	s) uselocalDIO="-dio";;
	t) queuedepth=$OPTARG;;
	esac
done
shift $((OPTIND-1))


if [ ! -d /dev/shm/condata ]; then
	mkdir /dev/shm/condata
	chmod 1777 /dev/shm/condata
fi

if [ ! -w /dev/shm/condata ]; then
	echo "/dev/shm/condata needs to be writeable"
	exit -1
fi


##########################################################################################
#Command Line Parameter   
##########################################################################################
HSourceIP=`hostname`           # HSourceIP - Source Host IP over which data is transferred
HDestinIP="$2"            # HDestinIP - Destin Host IP over which data is transferred
HSource=`hostname -s`     # assume Hostname==HostIP
TDSource="$1"             # complete filepath for source file
HDestin="$2"              # assume Hostname==HostIP
TDDestin="$3"             # complete filepath for destination file
#let "bytesperpass = ${5}" # Total bytes to transfer, do it in one XDD pass

colon_count=`echo $2 | grep -c ":"`

if [ $colon_count -eq 1 ]; then
	HDestin=`echo $2 | awk -F: '{print $1}'`
	HDestinIP=$HDestin
	TDDestin=`echo $2 | awk -F: '{print $2}'`
fi

if [ ! -e ${TDSource} ]; then
	echo "${TDSource} doesn't exist...will create it"
#	exit -1
fi

if [ $# -eq 4 ]; then
	bytesperpass=${4}
else
	if [ $TDSource = "/dev/null" -o $TDSource = "/dev/zero" ]; then
		bytesperpass=167772160
	else
		bytesperpass=`ls -l ${TDSource} | awk '{print $5}'`
	fi
fi

if [ $bytesperpass -lt 16777216 ]; then
	echo "xdd only works on files larger than 16777216"
	exit -1
fi

#notify mailing list that test is starting
#      mail -s "running xdd ${test} " -c durmstrang-io@ccs.ornl.gov ${USER}
#ssh ${HDestin} "mkdir -m 1777 /dev/shm/condata" 2> /dev/null

#################################################
#Hardwired stuff - change to suit
#################################################
E2EPORT1="-e2e port 40010"     # If ports get gummed up change them here
#useDIO="    "                 # To use Direct IO, or not (on the remote side)
#useDIO="-dio"                 # To use Direct IO, or not
#uselocalDIO=" "			# To use Direct IO on the local side
#let "queuedepth   = 4"       # queuedepth = number of I/O threads
#let "queuedepth   = 16"       # queuedepth = number of I/O threads
#let "xfer         = 1048576" # bytes per request per I/O thread
let "xfer         = 16777216" # bytes per request per I/O thread
##########################################################################
#DON'T CHANGE ANYTHING BELOW THIS LINE 
##########################################################################
let "numreqs      = $bytesperpass / $xfer" #numreqs is total number of requests of size blocksizeXreqsize=XFER 
let "blocksize    = 1024"
let "reqsize      = $xfer / $blocksize"
#let "reqsize      = 4096"

#################################################
# Always clean-up xdd garbage from previous tests
# Because we are testing, delete source/destination files
#################################################
#date
qipcrm ${USER}
#delete source file
rm -f ${TDSource}
pgrep -u ${USER} -f xdd | xargs kill -9 2> /dev/null
#delete destination file
ssh ${HDestin} "rm -f ${TDDestin}; qipcrm ${USER}; pgrep -u ${USER} -f xdd | xargs kill -9" 
#ssh ${HDestin} "mkdir -m 1777 /dev/shm/condata" 2> /dev/null

#################################################
# end-to-end: /dev/file-to-file
TSTAMP="`date +%y%m%d.%H%M%S`"
test="nt1.qd"$queuedepth"."$xfer"x"$numreqs"."$bytesperpass".xdd_${TSTAMP}"
#################################################

#ssh ${HDestin} xdd -targets 1 ${TDDestin} -minall -numreqs $numreqs  -reqsize $reqsize -blocksize $blocksize -verbose -queuedepth $queuedepth -op write -war writer ${HDestinIP} ${WARPORT1}  ${useDIO} &> ${test}.${HDestin}.out &

ssh ${HDestin} "xdd -targets 1 ${TDDestin} -minall -bytes $bytesperpass -reqsize $reqsize -verbose -queuedepth $queuedepth -op write -e2e isdestination -e2e dest ${HDestinIP} ${E2EPORT1}  ${useDIO} &> ${test}.${HDestin}.out &"

sleep 6

#ssh ${HSource} xdd -targets 1  ${TDSource} -minall -numreqs $numreqs -reqsize $reqsize -blocksize $blocksize -verbose -queuedepth $queuedepth -op read  -war writer ${HDestinIP} ${WARPORT1}  ${useDIO}  &> ${test}.${HSource}.out &

#xdd -targets 1  ${TDSource} -noconfidump -minall -bytes $bytesperpass -reqsize $reqsize -verbose -queuedepth $queuedepth -op read  -war writer ${HDestinIP} ${WARPORT1}  ${uselocalDIO}  -timerinfo #&> ${test}.${HSource}.out &
#########################
#make and move file
#########################
xdd -op write -datapattern random -bytes $bytesperpass -targets 1  ${TDSource}  > ${test}.${HSource}.out 2>&1
xdd -targets 1  ${TDSource} -minall -bytes $bytesperpass -reqsize $reqsize -queuedepth $queuedepth -op read -e2e issource -e2e dest ${HDestinIP} ${E2EPORT1}  ${uselocalDIO}  -timerinfo >> ${test}.${HSource}.out 2>&1 &

wait

#########################
#bring destination outputfile home
#########################
scp ${HDestin}:${HOME}/${test}.${HDestin}.out .

#########################
#check the files
#########################
                 (echo -n MD5SUM=;md5sum                             ${TDSource} )  >> ${test}.${HSource}.out 2>&1 &
ssh ${HDestin}  "(echo -n MD5SUM=;md5sum                             ${TDDestin} )" >> ${test}.${HDestin}.out 2>&1

wait
grep "MD5SUM=" ${test}*out


echo "TEST Transfer finished"
