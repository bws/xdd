#!/bin/sh

 SRC=$1
 SPATH=$2
 DST=$3
 DPATH=$4
 FILE=$5
          gather_all_info 2>&1 > info.src.$FILE
 ssh $DST gather_all_info 2>&1 > info.dst.$FILE
 ssh $DST rm $DPATH/\*
 md5sum $SPATH/$FILE 2>&1 > md5.$FILE
  cksum $SPATH/$FILE 2>&1 > cks.$FILE
 for n in 00 01 02 03 04 05 06 07 08 09
 do
 xddcp -f $SPATH/$FILE $DST:$DPATH/$FILE.$n
 sleep 5
 done
 scp $DST:$DPATH/xdd*log .
 ssh $DST md5sum   $DPATH/$FILE\* 2>&1 >> md5.$FILE 
 ssh $DST  cksum   $DPATH/$FILE\* 2>&1 >> cks.$FILE
 wait
