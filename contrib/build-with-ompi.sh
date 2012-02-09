#!/bin/bash

source /etc/profile.d/modules.sh
module load openmpi-x86_64
$@
exit $?
