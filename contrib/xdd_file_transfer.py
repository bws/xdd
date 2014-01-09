#!/bin/env python

#
# Stupid python-isms to make code slightly more portable
#
from __future__ import print_function

#
# Import modules
#
import sys
import xdd_settings
import xdd

class XDDTransferMasterException(Exception):
    """Error to allow alternative control flow"""
    pass
#
# Main
#
# Current algorithm is
#   For remote hosts, ensure that ssh access exists
#   For all hosts, check flow preconditions (permissions, etc.)
#   Start all XDD-based flows
#
#   If at any point an error occurs, go back and shut down anything that has
#   been successfuly started, and report the detected error to the user.
#   Also report anything that could not be successfully shutdown.
#
def main():
    # Create the TransferManager
    transferMgr = xdd.TransferManager()
    transferMgr.setRequestSize(8192)
    transferMgr.setTransferSize(1024*1024*1024)
    transferMgr.setSourceTarget("/dev/zero", dioFlag=False, serialFlag=False)
    transferMgr.setSinkTarget("/dev/null", dioFlag=False, serialFlag=False)
    transferMgr.addSource("localhost", 1)
    transferMgr.addSink("bws-mbpro2", 1)

    rc = 0
    try:
        # Use flow builder to spawn all flows
        rc = transferMgr.createFlows()
        if 0 != rc:
            print("ERROR: File data transfer failed during flow creation.")
            raise XDDTransferMasterException()

        # Start the flows
        rc = transferMgr.startFlows()
        if 0 != rc:
            print("ERROR: File data transfer failed during startup.")
            raise XDDTransferMasterException()


        # Monitor the flows at a 5 second interval
        rc = transferMgr.monitorFlows(5)
        if 0 != rc:
            print("ERROR: File data transfer failed during data movement.")
            raise XDDTransferMasterException()
    #except:
    #    print("Exception caught:", sys.exc_info())
    finally:
        # Stop all of the flows and get their status
        transferMgr.cleanupFlows()

    success = transferMgr.completedSuccessfully()
    if False == success:
        rc = 2
    return rc
    
# Execute main
if __name__ == "__main__":
    main()
