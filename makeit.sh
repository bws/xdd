#!/bin/sh
scp ../*${1}.tgz 192.168.40.130:~tmruwart/.  
ssh 192.168.40.130 "cd ~tmruwart/xdd ; tar xvfz ../*${1}.tgz ; make install "
