#!/bin/sh
for x in *.c *.h *.makefile
do
 tr -d '\r' < ${x} > y
 mv -f y ${x}
 echo " " >> ${x}
done
