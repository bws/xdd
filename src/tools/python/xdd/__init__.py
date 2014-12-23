"""
XDD Package.  Initialization stuff to determine if the installer has modified
paths and that the correct versions of all the needed packages exist.
"""
from __future__ import print_function
import sys

if sys.version_info < (2, 7):
    import warnings
    warnings.warn("XDD has not been tested on Python versions older than 2.7",
                  ImportWarning)

#
# If constants haven't been set from sentinels, then set them to pre-known
# development paths
#
from xdd.constants import XDD_CONFIG_PYRO_PORT, XDD_CONFIG_PYTHONPATH
if "@XDD_PYRO_PORT@" != XDD_CONFIG_PYRO_PORT:
    try:
        XDD_PYRO_PORT = Int(XDD_CONFIG_PYRO_PORT)
    except ValueError:
        print("ERROR Incorrect installation value for pyro port")
if "@XDD_PYTHONPATH@" != XDD_CONFIG_PYTHONPATH:
    XDD_PYTHONPATH = XDD_CONFIG_PYTHONPATH

#
# Import correct Xdd symbols
#
from xdd.profileparameters import ProfileParameters
from xdd.profiler import Profiler
from xdd.transfermanager import TransferManager
from xdd.constants import XDD_VERSION as __version__
