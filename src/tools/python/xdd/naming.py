#
# Python stuff to improve portability
#
from __future__ import print_function

#
# Python standard packages
#
import os

class NamingStrategy(object):
    """
    Base class for Naming Strategies that convert source-side directory, file,
    and symbolic link names into destination-side directory, file, and sybolic
    link names.
    """
    def buildDirsFilesLinks(self, source, target, targetExists, targetIsDir):
        """
        @return a (rc, dirs, files, symlinks) tuple
        rc is 0 on success
        dirs is the list of (sourcedir, targetdir) tuples
        files is a list of (sourcename, sinkname) tuples
        links is a list of (sourcelink, targetlink, linktarget) tuples
        """
        return (1, [], [], [])

class PosixNamingStrategy(NamingStrategy):
    """
    Translates source and destination names using the same exact strategy
    as the POSIX cp utility.  This class requires the source filesystem to 
    be present in order to descend the source file system and locate names
    recursively.  There is NO requirement that the destination file system
    be present or existent.

    A brief description of POSIX recursive file naming are as follows:
      cp -r source/ dest
        NOTE: The trailing slash is significant!!!
        if dest exists, the contents of source are placed in dest
        if dest does not exist, a new directory dest is created
        
      cp -r source dest
        if dest exists, a new directory dest/source is created
        if dest does not exist, a new directory dest is created

      Trailing slashes on the destination are not relevant.  Symbolic links 
      are created in the destination directory.  Absolute paths are re-written 
      for dest such that if possible, if the link targets contain part of the 
      source path, it will be changed to the dest path.
    """

    def walkError(self, e):
        """Raises the exception e"""
        raise e

    def buildLink(self, source, target, sourcePrefix, targetPrefix):
        """
        @return a (targetlink, linktarget) tuple
        """
        # Build the link target
        ltarget = os.readlink(source)
        return (source, target, ltarget)
           
    def buildDirsFilesLinks(self, source, target, targetExists, targetIsDir):
        """
        @return a (rc, dirs, files, symlinks) tuple
        rc is 0 on success
        dirs is the list of (sourcedir, targetdir) tuples
        files is a list of (sourcename, sinkname) tuples
        links is a list of (sourcelink, targetlink, linktarget) tuples
        """
        rc = 0
        outDirs = []
        outFiles = []
        outSymlinks = []

        # The source is a directory, link, or file.  Link has to be
        # checked first, because links may also be directories or files,
        # the final check does not use isfile() because we also need to
        # handle character special and block devices
        if os.path.islink(source):
            # Links are stored in target-value format
            value = os.readlink(source)
            # Build the target name
            if targetIsDir:
                base = os.path.basename(source)
                tname = os.path.join(target, base)
            else:
                tname = target
            outSymlinks = [(source, tname, value)]
        elif os.path.isdir(source) and (targetIsDir or not targetExists):
            # Determine if the source has a trailing slash
            (_, stail) = os.path.split(source)
            hasTrailingSlash = (0 == len(stail))

            # Build the target side prefix
            if targetExists and not hasTrailingSlash:
                targetPrefix = os.path.join(target, stail)
            else:
                targetPrefix = target

            # First add the target root level if needed
            if not targetExists or not hasTrailingSlash:
                outDirs.append( (source, targetPrefix) )

            # Perform the tree walk
            try:
                # Descend down the source path processing each dentry
                for dirpath, dirs, files in os.walk(source, topdown=True, 
                                                    onerror=self.walkError, 
                                                    followlinks=False):
                    # Strip off the parts of path supplied by caller
                    reldir = os.path.relpath(dirpath, start=source)

                    # Construct the directories and symlinks to directories
                    for d in dirs:
                        # Convert the name to use target prefix               
                        sourcename = os.path.join(dirpath, d)
                        targetname = os.path.join(targetPrefix, reldir, d)
                        # Handle symlinks
                        if os.path.islink(sourcename):
                            link = self.buildLink(sourcename, targetname, 
                                                  source, targetPrefix)
                            outSymlinks.append(link)
                        else:
                            outDirs.append( (sourcename, targetname) )
                    # Construct the files and symlinks to files
                    for f in files:
                        sourcename = os.path.join(dirpath, f)
                        targetname = os.path.join(targetPrefix, reldir, f)
                        # Handle symlinks
                        if os.path.islink(sourcename):
                            link = self.buildLink(sourcename, targetname, 
                                                  source, targetPrefix)
                            outSymlinks.append(link)
                        else:
                            outFiles.append( (sourcename, targetname) )
            except OSError as e:
                rc = 2
                outDirs = []
                outFiles = []
                outSymlinks = []

        elif not os.path.isdir(source) and os.path.exists(source):
            # Build the target name for regular and special files
            if targetIsDir:
                base = os.path.basename(source)
                tname = os.path.join(target, base)
            else:
                tname = target
            outFiles = [(source, tname)]
        else:
            rc = 1
                
        return (rc, outDirs, outFiles, outSymlinks)

class PosixPlusNamingStrategy(PosixNamingStrategy):
    """
    Translates source and destination names using the same strategy
    as the POSIX cp utility, with the exception that symlinks are translated
    to the degree possible from the source file system to the target file 
    system.  This class requires the source filesystem to 
    be present in order to descend the source file system and locate names
    recursively.  There is NO requirement that the destination file system
    be present or existent.
    """

    def buildLink(self, source, target, sourcePrefix, targetPrefix):
        """
        Convert links of the form 
          bar/baz -> /source/bar/foo 
        to
          bar/baz -> /target/bar/foo

        Convert links of the form
          bar/baz -> ../foo
        to
          bar/baz -> ../foo

        On links where that isn't the case, leave them as is

        @return a (targetlink, linktarget) tuple
        """
        # Build the link target
        ltarget = os.readlink(source)

        # Build the actual link path
        (ltargetRelDir, _) = os.path.split(source)
        lpath = os.path.join(ltargetRelDir, ltarget)

        # Remove relative path parts in parent directory portion of link target
        (ltargetDir, lfile) = os.path.split(ltarget)
        if ltargetDir:
            reltargetdir = os.path.relpath(ltargetDir, start=sourcePrefix)
        else:
            reltargetdir = ''
        reltarget = os.path.join(reltargetdir, lfile)                

        # Build the new target-side link tuple
        if ltarget != reltarget:
            link = (target, os.path.join(targetPrefix, reltarget))
        else:
            link = (target, ltarget)
        return link
           
