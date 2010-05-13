#!/usr/bin/env python
#
# Tester for mdtest
#
#/*****************************************************************************\
#*                                                                             *
#*       Copyright (c) 2003, The Regents of the University of California       *
#*     See the file COPYRIGHT for a complete copyright notice and license.     *
#*                                                                             *
#\*****************************************************************************/
#
# CVS info:
#   $RCSfile: tester.py,v $
#   $Revision: 1.4 $
#   $Date: 2006/08/09 22:13:13 $
#   $Author: loewe $

import sys
import os.path
import string
import time

debug = 0

# definitions
RMPOOL     = 'systest'
NODES      = 32
TPN        = 1
PROCS      = NODES * TPN
EXECUTABLE = './mdtest'
TEST_DIR_LOC = '/lustre/widow1/scratch/swh13/mdtests/testfiles'
FPP=1
ITERS=100
VERB='    '

# tests
tests = [



        # number of tasks to run, shared file, no task offset
        "-d " + TEST_DIR_LOC + " -n " + str(FPP) + " -i " + str(ITERS) + " -F -S -N " + str(0) + " -f 1 -l " \
            + str(PROCS-1) + " -s " + str(PROCS/3) + VERB,


        # number of tasks to run, shared file, no task offset
        "-d " + TEST_DIR_LOC + " -n " + str(FPP) + " -i " + str(ITERS) + "    -S -N " + str(0) + " -f 1 -l " \
            + str(PROCS-1) + " -s " + str(PROCS/3) + VERB,

        # number of tasks to run, shared file, one-node task offset
       # "-d " + TEST_DIR_LOC + " -n " + str(FPP) + " -i " + str(ITERS) + " -S -N " + str(TPN) + " -f 1 -l " \
       #     + str(PROCS-1) + " -s " + str(PROCS/3) + VERB,

       # # number of tasks to run, shared file, one-node task offset, remove files from previous test
       # "-d " + TEST_DIR_LOC + " -n " + str(FPP) + " -i " + str(ITERS) + " -S -N " + str(TPN) + " -f 1 -l " \
       #     + str(PROCS-1) + " -s " + str(PROCS/3) + VERB + " -r ",

       # # number of tasks to run, file per proc, no task offset
       # "-d " + TEST_DIR_LOC + " -n " + str(FPP) + " -i " + str(ITERS) + "    -N " + str(0) + " -f 1 -l " \
       #     + str(PROCS-1) + " -s " + str(PROCS/3) + VERB,

       # # number of tasks to run, file per proc, no task offset, remove files from previous test
       # "-d " + TEST_DIR_LOC + " -n " + str(FPP) + " -i " + str(ITERS) + "    -N " + str(0) + " -f 1 -l " \
       #     + str(PROCS-1) + " -s " + str(PROCS/3) + VERB + " -r ",

        # number of tasks to run, file per proc, one-node offset
       # "-d " + TEST_DIR_LOC + " -n " + str(FPP) + " -i " + str(ITERS) + "     -N " + str(TPN) + " -f 1 -l " \
       #     + str(PROCS-1) + " -s " + str(PROCS/3) + VERB,

        # remove any remaining tests from previous run
       # "-d " + TEST_DIR_LOC + " -n " + str(FPP) + " -i " + str(ITERS) + "     -N " + str(TPN) + " -f 1 -l " \
       #     + str(PROCS-1) + " -s " + str(PROCS/3) + VERB + " -r "
]


#############################
# set environment variables #
#############################
def SetEnvironment(rmpool, nodes, procs):
    os.environ['MP_RMPOOL'] = str(rmpool)
    os.environ['MP_NODES']  = str(nodes)
    os.environ['MP_PROCS']  = str(procs)
    return


#################
# flush to file #
#################
def Flush2File(resultsFile, string):
    resultsFile.write(string + '\n')
    resultsFile.flush()


###################
# run test script #
###################
def RunScript(resultsFile, test):
    # -- for poe --   command = "poe " + EXECUTABLE + " " + test
    command = "aprun -n " + str(PROCS) + " -N " + str(TPN) + " " + EXECUTABLE + " " + test
    if debug == 1:
        Flush2File(resultsFile, command)
    else:
        childIn, childOut = os.popen4(command)
        childIn.close()
        while 1:
            line = childOut.readline()
            if line == '': break
            Flush2File(resultsFile, line[:-1])
        childOut.close()
    return


########
# main #                                                                       #
########
def main():
    resultsFile = open("./results.txt-" + \
                       os.popen("date +%m.%d.%y").read()[:-1], "w")

    Flush2File(resultsFile, "Testing mdtest")

    # test -h option on one task
#    SetEnvironment(RMPOOL, 1, 1)
#   RunScript(resultsFile, '-h')

    # set environ and run tests
    SetEnvironment(RMPOOL, NODES, PROCS)
    for i in range(0, len(tests)):
        time.sleep(0)                     # delay any cleanup for previous test
        RunScript(resultsFile, tests[i])
            
    Flush2File(resultsFile, "\nFinished testing mdtest")
    resultsFile.close()

if __name__ == "__main__":
    main()

