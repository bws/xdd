#!/usr/bin/gnuplot
set term po eps color
set pointsize 2
set ylabel "MB/s"
set xlabel "Time in seconds"
set title "XDDCP Transfer"
set style data line

infile='out6.dat'
set output 'xddcp6_xfer_all.eps'
plot	infile using 1:2 title 'read_disk' lw 2,\
	infile using 3:4 title 'send_net' lw 2,\
	infile using 5:6 title 'recv_net' lw 2,\
	infile using 7:8 title 'write_disk' lw 2

set output 'xddcp6_xfer_net.eps'
plot	infile using 3:4 title 'send_net' lw 2,\
	infile using 5:6 title 'recv_net' lw 2

set output 'xddcp6_xfer_disk.eps'
plot	infile using 1:2 title 'read_disk' lw 2,\
	infile using 7:8 title 'write_disk' lw 2

set output 'xddcp6_xfer_src.eps'
plot	infile using 1:2 title 'read_disk' lw 2,\
	infile using 3:4 title 'send_net' lw 2

set output 'xddcp6_xfer_dest.eps'
plot	infile using 5:6 title 'recv_net' lw 2,\
	infile using 7:8 title 'write_disk' lw 2

