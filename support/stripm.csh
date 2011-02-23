#!/bin/csh
foreach x ( *.c *.h *.makefile )
 tr -d '\r' < ${x} > y
 mv -f y ${x}
 echo " " >> ${x}
end
