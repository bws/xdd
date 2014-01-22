"""
XDD RPC Listen server.  This server exports XDD's python object's via RPC.
"""
from __future__ import print_function
import os
import sys
import threading
import time

#
# Packages supplied with XDD
#
import Pyro4
from xdd import constants, flowbuilder

def ppidCheck(server):
    """
    Check to see if the parent PID is 1, and if it is, call Pyro4.shutdown()
    """
    # The client to the RPC server cannot guarantee that it can send the
    # terminate signal to the RPC server, however, it will kill the parent
    # process when the SSH connection terminates.  When the happens, the
    # parent process will become init (PID: 1), and this thread will then
    # shutdown the Pyro server cleanly
    while True:
        if 1 == os.getppid():
            server.shutdown()
            break
        time.sleep(5)

def rpc_server():
    """Main to run an actual XDD Flow Server"""

    # Create the server
    pyroServer = None
    try:
        # Let the daemon find a port on its own
        pyroServer = Pyro4.Daemon(host='localhost', port=0)
    except socket.error:
        print("Pyro connection error.")

    except:
        print("Pyro unknown error.")
        e = sys.exc_info()[0]
        print(str(e))
        
    flowBuilder = flowbuilder.RemoteFlowBuilder(pyroServer)
    uri = pyroServer.register(flowBuilder)

    # Publish the URI over stdout
    print(constants.XDD_PYRO_URI_DELIMITER)
    sys.stdout.flush()
    print (uri)
    sys.stdout.flush()
    print(constants.XDD_PYRO_URI_DELIMITER)
    sys.stdout.flush()
    sys.stdout.flush()

    # Start the monitor thread
    monitor = threading.Thread(target = ppidCheck, args = (pyroServer, ))
    monitor.daemon = True
    monitor.start()

    # Daemonize and wait for requests
    pyroServer.requestLoop()

    return 0


if __name__ == '__main__':
    rpc_server()

