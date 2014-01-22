"""
Core functionality for use thoughout the XDD package
"""

#
# Root exception class
#
class XDDError(Exception):
    """Base class for exceptions in XDD package."""
    pass

#
# XDD transfer exceptions
#
class TransferPreconditionError(XDDError):
    """Exception raised when a flow is unable to be started."""
    def __init__(self, reasons):
        self.reasons = reasons

class TransferRemoteError(XDDError):
    """Exceptions raised when the proxy is unable to contact remote."""
    pass

class TransferSinkPreconditionError(TransferPreconditionError):
    """Exception raised when a sink flow fails during startup."""
    pass

class TransferSourcePreconditionError(TransferPreconditionError):
    """Exception raised when a source flow fails during startup."""
    pass

