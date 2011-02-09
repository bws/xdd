
set term po eps color
set pointsize 2
set ylabel "MB/s"
set xlabel "Time in seconds"
set title "XDDCP Transfer"
set style data line

infile='tsdump.dat'
set output 'xddcp_xfer.eps'
plot	infile using 1:2 title 'read_disk',\
	infile using 3:4 title 'send_net',\
	infile using 5:6 title 'recv_net',\
	infile using 7:8 title 'write_disk'

