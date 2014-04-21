#
# Constants for XDD's Python modules
#
from __future__ import print_function

#
# Settings modified during install
#
XDD_VERSION = 'alpha'
XDD_CONFIG_PYTHONPATH = "@XDD_PYTHONPATH@"
XDD_CONFIG_PYRO_PORT = "@XDD_PYRO_PORT@"

#
# Setup the pyro port
#
XDD_PYRO_PORT = 40010
if "@XDD_PYRO_PORT@" != XDD_CONFIG_PYRO_PORT:
    try:
        XDD_PYRO_PORT = Int(XDD_CONFIG_PYRO_PORT)
    except ValueError:
        print("ERROR Incorrect installation value for pyro port")

#
# Setup the python path
#
XDD_PYTHONPATH = "workspace/xdd/src/tools/python"
if "@XDD_PYTHONPATH@" != XDD_CONFIG_PYTHONPATH:
    XDD_PYTHONPATH = XDD_CONFIG_PYTHONPATH

#
# Set the Pyro URI delimiter
#
XDD_PYRO_URI_DELIMITER = ":::XDD URI:::"
XDD_PYRO_ACK = "ACK\n"
XDD_PYRO_NACK = "NACK\n"

#
# Token exported inside XDD that indicates XFS preallocation is available
#
XDD_PREALLOC_TOKEN = 'xgp_xfs_enabled'

#
# XDD Error Codes
#
XDD_SUCCESS = 0
XDD_BAD_ARGS = 1
XDD_SERVER_CONNECT_FAILED = 2
XDD_CLIENT_CONNECT_FAILED = 4

#
# XDD Settings
#
XDD_SINK_TO_SOURCE_DELAY = 0.8
XDD_RESTART_COOKIE_EXTENSION = 'xrf'
XDD_PROGRESS_FILENAME = '.xddmcp.xpg'
