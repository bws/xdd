
#
# Python stuff to improve portability
#
from __future__ import print_function

#
# Generic python packages used
#
from collections import OrderedDict
from math import ceil, log10, floor, pow
import os
import stat
import subprocess
import sqlite3
import time

#
#
#
class ProfileVisualizer:
    """
    ProfileVisualizer for generating graphical representations of a storage
    profile.
    """

    class Output:
        EPS, PDF, PNG = range(3)
        
    def __init__(self, results, outputDir, pformat=Output.PNG):
        """Constructor"""
        self._outputDir = outputDir
        self._output = pformat
        self._results = results
        self._sqllog = outputDir + os.sep + 'trials.db3'
        self._maxGiBs = None
        self._maxIOPs = None
        self._maxReqsize = None
        self._maxThreads = None
        
    def plotAll(self):
        """Create all I/O plots using Gnuplot and Sqlite"""
        self._convertTSVtoDB2()
        self._generateBandwidthGraphs()
        self._generateIOPGraphs()
        
    def _convertTSVtoDB2(self):
        """Convert TSV output into a sqllite DB file"""
        conn = sqlite3.connect(self._sqllog)
        c = conn.cursor()

        # Create the table
        columns = [('uid', 'integer'),
                   ('optrial', 'integer'),
                   ('numvols', 'integer'),
                   ('op', 'text'),
                   ('trial', 'integer'),
                   ('target', 'text'),
                   ('tlimit', 'integer'),
                   ('reqsz', 'integer'),
                   ('threads', 'integer'),
                   ('direct', 'integer'),
                   ('issue', 'text'),
                   ('pattern', 'text'),
                   ('alloc', 'text'),
                   ('bytes', 'integer'),
                   ('tops', 'integer'),
                   ('secs', 'real'),
                   ('iops', 'real'),
                   ('MBs', 'real'),
                   ('GBs', 'real'),
                   ('MiBs', 'real'),
                   ('GiBs', 'real')]
        createstmt = 'CREATE TABLE trials ('
        for n,t in columns:
            createstmt += n + ' ' + t + ', '
        createstmt = createstmt[:-2]
        createstmt += ')'
        #print(createstmt)
        c.execute(createstmt)
        conn.commit()
        
        # Insert all of the rows
        c = conn.cursor()
        uid = 0
        for result in self._results:
            insertstmt = 'INSERT INTO trials VALUES (' + str(uid) + ', '
            for val in result:
                insertstmt += '\'' + val + '\', '
            insertstmt = insertstmt[:-2]
            insertstmt += ')'
            #print(insertstmt)
            c.execute(insertstmt)
            uid += 1
        conn.commit()
        conn.close()

    def _generateBandwidthGraphs(self):
        """Generate files containing bandwidth graphs"""
        self._plotSeqWriteBandwidthSingleThread('bw-wr-1-thread')
        self._plotSeqReadBandwidthSingleThread('bw-rd-1-thread')
        self._plotSeqWriteBandwidthThreads('bw-wr-threads')
        self._plotSeqReadBandwidthThreads('bw-rd-threads')
        self._plotSeqWriteBandwidthThreadsIssue('bw-wr-threads-issue')
        self._plotSeqReadBandwidthThreadsIssue('bw-rd-threads-issue')
        
    def _generateIOPGraphs(self):
        """Generate files containing IOP graphs"""
        self._plotRandomWriteOpsSingleThread('iop-wr-1-thread')
        self._plotRandomReadOpsSingleThread('iop-rd-1-thread')
        self._plotRandomWriteOpsThreads('iop-4k-wr-threads')
        self._plotRandomReadOpsThreads('iop-4k-rd-threads')

    def _plotSeqWriteBandwidthSingleThread(self, fname):
        """ Compare Direct vs Buffered Seq Writes for 1 thread """
        # Build buffered query
        wherebuffered = 'WHERE reqsz >= 524288 and threads=1 and ' + \
                        'direct=\\"False\\" and ' + \
                        'op=\\"write\\" and ' + \
                        'issue=\\"serial\\" and  ' + \
                        'pattern=\\"seq\\" and ' + \
                        'alloc=\\"demand\\"'
        bufferedbwmin = 'SELECT min(gibs) FROM trials ' + wherebuffered
        bufferedbwmax = 'SELECT max(gibs) FROM trials ' + wherebuffered
        bufferedwbw = 'SELECT reqsz, gibs' + \
                      ', (' +  bufferedbwmin + '), (' + bufferedbwmax + ') ' + \
                      'FROM trials ' + wherebuffered

        # Build direct query
        wheredirect = 'WHERE reqsz >= 524288 and threads=1 and ' + \
                      'direct=\\"True\\" and ' + \
                      'op=\\"write\\" and ' + \
                      'issue=\\"serial\\" and  ' + \
                      'pattern=\\"seq\\" and ' + \
                      'alloc=\\"demand\\"'
        directbwmin = 'SELECT min(gibs) FROM trials ' + wheredirect
        directbwmax = 'SELECT max(gibs) FROM trials ' + wheredirect
        directwbw = 'SELECT reqsz, gibs ' + \
                      ', (' +  directbwmin + '), (' + directbwmax + ') ' + \
                      'FROM trials ' + wheredirect
        
        sql = '< sqlite3 ' + self._sqllog + ' '
        data1 = sql + '\'' + bufferedwbw + '\''
        data2 = sql + '\'' + directwbw + '\''
        wpairs = { '1 Buffered I/O Seq. Writer' : data1,
                   '1 Direct I/O Seq. Writer' : data2 }
        self.graphReqsizeBandwidth(fname, wpairs)
        
    def _plotSeqReadBandwidthSingleThread(self, fname):
        """ Compare Direct vs Buffered Seq Reads for 1 thread """
        bufferedrbw = 'SELECT reqsz, gibs ' +  \
                        'FROM trials ' + \
                        'WHERE reqsz >= 524288 and threads=1 and ' + \
                        'direct=\\"False\\" and ' + \
                        'op=\\"read\\" and ' + \
                        'issue=\\"serial\\" and  ' + \
                        'pattern=\\"seq\\" and ' + \
                        'alloc=\\"demand\\"'
        directrbw = 'SELECT reqsz, gibs ' +  \
                      'FROM trials ' + \
                      'WHERE reqsz >= 524288 and threads=1 and ' + \
                      'direct=\\"True\\" and ' + \
                      'op=\\"read\\" and ' + \
                      'issue=\\"serial\\" and  ' + \
                      'pattern=\\"seq\\" and ' + \
                      'alloc=\\"demand\\"'
        sql = '< sqlite3 ' + self._sqllog + ' '
        data1 = sql + '\'' + bufferedrbw + '\''
        data2 = sql + '\'' + directrbw + '\''
        rpairs = { '1 Buffered I/O Seq. Writer' : data1,
                   '1 Direct I/O Seq. Writer' : data2 }
        self.graphReqsizeBandwidth(fname, rpairs)

    def _plotSeqWriteBandwidthThreads(self, fname):
        """Compare Direct vs Buffered Seq Writes for threads"""
        bufferedwbw = 'SELECT threads, gibs ' +  \
                        'FROM trials ' + \
                        'WHERE reqsz = 4096*1024 and ' + \
                        'direct=\\"False\\" and ' + \
                        'op=\\"write\\" and ' + \
                        'issue=\\"serial\\" and  ' + \
                        'pattern=\\"seq\\" and ' + \
                        'alloc=\\"demand\\"'
        directwbw = 'SELECT threads, gibs ' +  \
                      'FROM trials ' + \
                      'WHERE reqsz = 4096*1024 and ' + \
                      'direct=\\"True\\" and ' + \
                      'op=\\"write\\" and ' + \
                      'issue=\\"serial\\" and  ' + \
                      'pattern=\\"seq\\" and ' + \
                      'alloc=\\"demand\\"'
        sql = '< sqlite3 ' + self._sqllog + ' '
        data1 = sql + '\'' + bufferedwbw + '\''
        data2 = sql + '\'' + directwbw + '\''
        wpairs = { 'Buffered I/O Seq. 4MiB Writers' : data1,
                   'Direct I/O Seq. 4MiB Writers' : data2 }
        self.graphThreadsBandwidth(fname, wpairs)

    def _plotSeqReadBandwidthThreads(self, fname):
        """ Compare Direct vs Buffered Seq Reads for threads """
        bufferedwbw = 'SELECT threads, gibs ' +  \
                        'FROM trials ' + \
                        'WHERE reqsz = 4096*1024 and ' + \
                        'direct=\\"False\\" and ' + \
                        'op=\\"read\\" and ' + \
                        'issue=\\"serial\\" and  ' + \
                        'pattern=\\"seq\\" and ' + \
                        'alloc=\\"demand\\"'
        directwbw = 'SELECT threads, gibs ' +  \
                      'FROM trials ' + \
                      'WHERE reqsz = 4096*1024 and ' + \
                      'direct=\\"True\\" and ' + \
                      'op=\\"read\\" and ' + \
                      'issue=\\"serial\\" and  ' + \
                      'pattern=\\"seq\\" and ' + \
                      'alloc=\\"demand\\"'
        sql = '< sqlite3 ' + self._sqllog + ' '
        data1 = sql + '\'' + bufferedwbw + '\''
        data2 = sql + '\'' + directwbw + '\''
        wpairs = { 'Buffered I/O Seq. 4MiB Readers' : data1,
                   'Direct I/O Seq. 4MiB Readers' : data2 }
        self.graphThreadsBandwidth(fname, wpairs)

    def _plotSeqWriteBandwidthThreadsIssue(self, fname):
        """ Compare Issue order Seq Writes for threads """
        loosewbw = 'SELECT threads, gibs ' +  \
                        'FROM trials ' + \
                        'WHERE reqsz = 4096*1024 and ' + \
                        'direct=\\"True\\" and ' + \
                        'op=\\"write\\" and ' + \
                        'issue=\\"loose\\" and  ' + \
                        'pattern=\\"seq\\" and ' + \
                        'alloc=\\"demand\\"'
        serialwbw = 'SELECT reqsz, gibs ' +  \
                      'FROM trials ' + \
                      'WHERE reqsz = 4096*1024 and ' + \
                      'direct=\\"True\\" and ' + \
                      'op=\\"write\\" and ' + \
                      'issue=\\"serial\\" and  ' + \
                      'pattern=\\"seq\\" and ' + \
                      'alloc=\\"demand\\"'
        sql = '< sqlite3 ' + self._sqllog + ' '
        data1 = sql + '\'' + loosewbw + '\''
        data2 = sql + '\'' + serialwbw + '\''
        pairs = { 'Direct I/O Seq. 4MiB Writers (loose)' : data1,
                  'Direct I/O Seq. 4MiB Writers (serial)' : data2 }
        self.graphThreadsBandwidth(fname, pairs)

    def _plotSeqReadBandwidthThreadsIssue(self, fname):
        """ Compare Issue order Seq Reads for threads """
        loosewbw = 'SELECT threads, gibs ' +  \
                        'FROM trials ' + \
                        'WHERE reqsz = 4096*1024 and ' + \
                        'direct=\\"True\\" and ' + \
                        'op=\\"read\\" and ' + \
                        'issue=\\"loose\\" and  ' + \
                      'pattern=\\"seq\\" and ' + \
                      'alloc=\\"demand\\"'
        serialwbw = 'SELECT threads, gibs ' +  \
                      'FROM trials ' + \
                      'WHERE reqsz = 4096*1024 and ' + \
                      'direct=\\"True\\" and ' + \
                      'op=\\"read\\" and ' + \
                      'issue=\\"serial\\" and ' + \
                      'pattern=\\"seq\\" and ' + \
                      'alloc=\\"demand\\"'
        sql = '< sqlite3 ' + self._sqllog + ' '
        data1 = sql + '\'' + loosewbw + '\''
        data2 = sql + '\'' + serialwbw + '\''
        pairs = { 'Direct I/O Seq. 4MiB Readers (loose)' : data1,
                  'Direct I/O Seq. 4MiB Readers (serial)' : data2 }
        self.graphThreadsBandwidth(fname, pairs)

    def _plotRandomWriteOpsSingleThread(self, fname):
        """ Compare Direct vs Buffered Random Writes for 1 thread """
        bufferedwiops = 'SELECT reqsz, iops ' +  \
                        'FROM trials ' + \
                        'WHERE reqsz <= 524288 and threads=1 and ' + \
                        'direct=\\"False\\" and ' + \
                        'op=\\"write\\" and ' + \
                        'issue=\\"none\\" and ' + \
                        'pattern=\\"random\\" and ' + \
                        'alloc=\\"demand\\"'
        directwiops = 'SELECT reqsz, iops ' +  \
                      'FROM trials ' + \
                      'WHERE reqsz <= 524288 and threads=1 and ' + \
                      'direct=\\"True\\" and ' + \
                      'op=\\"write\\" and ' + \
                      'issue=\\"none\\" and ' + \
                      'pattern=\\"random\\" and ' + \
                      'alloc=\\"demand\\"'
        sql = '< sqlite3 ' + self._sqllog + ' '
        data1 = sql + '\'' + bufferedwiops + '\''
        data2 = sql + '\'' + directwiops + '\''
        wpairs = { '1 Buffered I/O Random Writer' : data1,
                   '1 Direct I/O Random Writer' : data2 }
        self.graphReqsizeOps(fname, wpairs)

    def _plotRandomReadOpsSingleThread(self, fname):
        """ Compare Direct vs Buffered Random Reads for 1 thread """
        bufferedwiops = 'SELECT reqsz, iops ' +  \
                        'FROM trials ' + \
                        'WHERE reqsz <= 524288 and threads=1 and ' + \
                        'direct=\\"False\\" and ' + \
                        'op=\\"read\\" and ' + \
                        'issue=\\"none\\" and ' + \
                        'pattern=\\"random\\" and ' + \
                        'alloc=\\"demand\\"'
        directwiops = 'SELECT reqsz, iops ' +  \
                      'FROM trials ' + \
                      'WHERE reqsz <= 524288 and threads=1 and ' + \
                      'direct=\\"True\\" and ' + \
                      'op=\\"read\\" and ' + \
                      'issue=\\"none\\" and ' + \
                      'pattern=\\"random\\" and ' + \
                      'alloc=\\"demand\\"'
        sql = '< sqlite3 ' + self._sqllog + ' '
        data1 = sql + '\'' + bufferedwiops + '\''
        data2 = sql + '\'' + directwiops + '\''
        wpairs = { '1 Buffered I/O Random Reader' : data1,
                   '1 Direct I/O Random Reader' : data2 }
        self.graphReqsizeOps(fname, wpairs)

    def _plotRandomWriteOpsThreads(self, fname):
        """ Compare Direct vs Random Writes for threads """
        bufferedwiops = 'SELECT threads, iops ' +  \
                        'FROM trials ' + \
                        'WHERE reqsz = 4096  and ' + \
                        'direct=\\"False\\" and ' + \
                        'op=\\"write\\" and ' + \
                        'issue=\\"none\\" and ' + \
                        'pattern=\\"random\\" and ' + \
                        'alloc=\\"demand\\"'
        directwiops = 'SELECT threads, iops ' +  \
                      'FROM trials ' + \
                      'WHERE reqsz = 4096 and ' + \
                      'direct=\\"True\\" and ' + \
                      'op=\\"write\\" and ' + \
                      'issue=\\"none\\" and ' + \
                      'pattern=\\"random\\" and ' + \
                      'alloc=\\"demand\\"'
        sql = '< sqlite3 ' + self._sqllog + ' '
        data1 = sql + '\'' + bufferedwiops + '\''
        data2 = sql + '\'' + directwiops + '\''
        wpairs = { 'Buffered I/O Random Writers' : data1,
                   'Direct I/O Random Writers' : data2 }
        self.graphThreadsOps(fname, wpairs)

    def _plotRandomReadOpsThreads(self, fname):
        """ Compare Direct vs Buffered Random Reads for threads """
        bufferedriops = 'SELECT threads, iops ' +  \
                        'FROM trials ' + \
                        'WHERE reqsz = 4096 and ' + \
                        'direct=\\"False\\" and ' + \
                        'op=\\"read\\" and ' + \
                        'issue=\\"none\\" and ' + \
                        'pattern=\\"random\\" and ' + \
                        'alloc=\\"demand\\"'
        directriops = 'SELECT threads, iops ' +  \
                      'FROM trials ' + \
                      'WHERE reqsz = 4096 and ' + \
                      'direct=\\"True\\" and ' + \
                      'op=\\"read\\" and ' + \
                      'issue=\\"none\\" and ' + \
                      'pattern=\\"random\\" and ' + \
                      'alloc=\\"demand\\"'
        sql = '< sqlite3 ' + self._sqllog + ' '
        data1 = sql + '\'' + bufferedriops + '\''
        data2 = sql + '\'' + directriops + '\''
        rpairs = { 'Buffered I/O Random Readers' : data1,
                   'Direct I/O Random Readers' : data2 }
        self.graphThreadsOps(fname, rpairs)
        
    def graphReqsizeOps(self, prefix, titlesFiles):
        """ """
        fname = self._outputDir + os.sep + prefix + '.plt'
        graphfile = self._outputDir + os.sep + prefix + self._getGraphExt()
        contents = self._getReqsizeOpsGnuplot(graphfile, titlesFiles)
        fh = open(fname, 'w')
        fh.write(contents)
        fh.close()
        st = os.stat(fname)
        os.chmod(fname, st.st_mode | stat.S_IEXEC)
        subprocess.call(fname)
        
    def graphThreadsOps(self, prefix, titlesFiles):
        """ """
        fname = self._outputDir + os.sep + prefix + '.plt'
        graphfile = self._outputDir + os.sep + prefix + self._getGraphExt()
        contents = self._getThreadsOpsGnuplot(graphfile, titlesFiles)
        fh = open(fname, 'w')
        fh.write(contents)
        fh.close()
        st = os.stat(fname)
        os.chmod(fname, st.st_mode | stat.S_IEXEC)
        subprocess.call(fname)
        
    def graphReqsizeBandwidth(self, prefix, titlesFiles):
        """ """
        fname = self._outputDir + os.sep + prefix + '.plt'
        graphfile = self._outputDir + os.sep + prefix + self._getGraphExt()
        contents = self._getReqsizeBWGnuplot(graphfile, titlesFiles)
        fh = open(fname, 'w')
        fh.write(contents)
        fh.close()
        st = os.stat(fname)
        os.chmod(fname, st.st_mode | stat.S_IEXEC)
        subprocess.call(fname)
        
    def graphThreadsBandwidth(self, prefix, titlesFiles):
        """ """
        fname = self._outputDir + os.sep + prefix + '.plt'
        graphfile = self._outputDir + os.sep + prefix + self._getGraphExt()
        contents = self._getThreadsBWGnuplot(graphfile, titlesFiles)
        fh = open(fname, 'w')
        fh.write(contents)
        fh.close()
        st = os.stat(fname)
        os.chmod(fname, st.st_mode | stat.S_IEXEC)
        subprocess.call(fname)
        
    def _getReqsizeOpsGnuplot(self, graphfile, titleFilePairs):
        """Graph Reqsize vs IOPs for data in filename"""
        f = self._getHeader() + os.linesep
        f += self._getTerminal() + os.linesep
        f += self._getStyle() + os.linesep
        f += self._getReqsizeIOPsAxes() + os.linesep
        f += 'set output "' + graphfile + '"' + os.linesep
        
        # Generate plot statements for data files
        f += 'plot \\' + os.linesep
        ls=1
        for title, df in titleFilePairs.items():
            f += '  "' + df + '" using ($1/1024):2 '
            f += ' title "' + title + '" with linespoints ls ' + str(ls)
            f += ', \\'
            f += os.linesep
            ls += 1
        # Chop off the last 3 characters when returning
        return f[:-4]
        
    def _getReqsizeBWGnuplot(self, graphfile, titleFilePairs):
        """Graph Reqsize vs BW for data in filename"""
        f = self._getHeader() + os.linesep
        f += self._getTerminal() + os.linesep
        f += self._getStyle() + os.linesep
        f += self._getReqsizeGiBsAxes() + os.linesep
        f += 'set output "' + graphfile + '"' + os.linesep

        # Generate plot statements for data files
        f += 'plot \\' + os.linesep
        ls=1
        for title, df in titleFilePairs.items():
            f += '  "' + df + '" using ($1/1024/1024):2 '
            f += ' title "' + title + '" with linespoints ls ' + str(ls)
            f += ', \\'
            f += os.linesep
            ls += 1
        # Chop off the last 3 characters when returning
        return f[:-4]
        
    def _getThreadsOpsGnuplot(self, graphfile, titleFilePairs):
        """Graph Threads vs IOPs for data in filename"""
        f = self._getHeader() + os.linesep
        f += self._getTerminal() + os.linesep
        f += self._getStyle() + os.linesep
        f += self._getThreadsIOPsAxes() + os.linesep       
        f += 'set output "' + graphfile + '"' + os.linesep

        # Generate plot statements for data files
        f += 'plot \\' + os.linesep
        ls=1
        for title, df in titleFilePairs.items():
            f += '  "' + df + '" using 1:2 '
            f += ' title "' + title + '" with linespoints ls ' + str(ls)
            f += ', \\'
            f += os.linesep
            ls += 1
        # Chop off the last 3 characters when returning
        return f[:-4]
        
    def _getThreadsBWGnuplot(self, graphfile, titleFilePairs):
        """Graph Threads vs Bandwidth for data in filename"""
        f = self._getHeader() + os.linesep
        f += self._getTerminal() + os.linesep
        f += self._getStyle() + os.linesep
        f += self._getThreadsGiBsAxes() + os.linesep
        f += 'set output "' + graphfile + '"' + os.linesep

        # Generate plot statements for data files
        f += 'plot \\' + os.linesep
        ls=1
        for title, df in titleFilePairs.items():
            f += '  "' + df + '" using 1:2 '
            f += ' title "' + title + '" with linespoints ls ' + str(ls)
            f += ', \\'
            f += os.linesep
            ls += 1
        # Chop off the last 3 characters when returning
        return f[:-4]
        
    def _getGraphExt(self):
        """@return the filename extension for the output graph"""
        ext = ''
        if self._output == ProfileVisualizer.Output.EPS:
            ext = '.eps'
        elif self._output == ProfileVisualizer.Output.PDF:
            ext = '.pdf'
        elif self._output == ProfileVisualizer.Output.PNG:
            ext = '.png'
        return ext

    def _getHeader(self):
        """@return GNUPlot header string"""
        return '#!/usr/bin/env gnuplot'
    
    def _getTerminal(self):
        """@return terminal specifier"""
        ts = os.linesep
        if self._output == ProfileVisualizer.Output.EPS:
            ts = 'set terminal postscript enhanced'
        elif self._output == ProfileVisualizer.Output.PDF:
            ts = 'set terminal pdf'
        elif self._output == ProfileVisualizer.Output.PNG:
            ts = 'set terminal pngcairo'
        return ts

    def _getStyle(self):
        ss = os.linesep
        ss += 'set style line  1 lt 3 lw 2 lc rgb "#8b1a0e"' + os.linesep 
        ss += 'set style line  2 lt 3 lw 2 lc rgb "#5e9c36"' + os.linesep
        ss += 'set style line  3 lt 3 lw 2 lc 3' + os.linesep
        ss += 'set style line  4 lt 3 lw 2 lc 4' + os.linesep
        ss += 'set style line  5 lt 3 lw 2 lc 5' + os.linesep
        ss += 'set style line  6 lt 3 lw 2 lc 6' + os.linesep
        ss += 'set style line 11 lt 1 lw 1 lc rgb "#808080"'  + os.linesep 
        ss += 'set style line 12 lt 0 lw 1 lc rgb "#808080"'  + os.linesep
        ss += 'set border 3 back ls 11' + os.linesep
        ss += 'set grid back ls 12' + os.linesep
        ss += 'set tics nomirror' + os.linesep
        ss += 'set datafile separator "|"' + os.linesep
        return ss

    def _getReqsizeIOPsAxes(self):
        """@return """
        # Round the reqsize up
        maxReq = self._getMaxReqsize() / 1024
        tens = floor(log10(float(maxReq)))
        reqRange = int(ceil(maxReq/pow(10, tens)) * pow(10,tens))

        ria = os.linesep
        # Set the X and Y axis labels
        ria += 'set ylabel "IOPs"' + os.linesep
        ria += 'set xlabel "Req. Size (KiB)"' + os.linesep
        ria += 'set xtics 0,1024,16384' + os.linesep
        # Set X and Y ranges
        ria += 'set yrange [0:' + str(self._getMaxIOPs() + 1000) + ']' + os.linesep
        ria += 'set xrange [0:520]'+ os.linesep
        return ria
    
    def _getThreadsIOPsAxes(self):
        """@return """
        tia = os.linesep
        # Set the X and Y axis labels
        tia += 'set ylabel "IOPs"' + os.linesep
        tia += 'set xlabel "Threads"' + os.linesep
        tia += 'set xtics 0,1,64' + os.linesep
        # Set X and Y ranges
        tia += 'set yrange [0:' + str(self._getMaxIOPs() + 1000) + ']' + os.linesep
        tia += 'set xrange [0:' + str(self._getMaxThreads() + 1) + ']' + os.linesep        
        return tia
    
    def _getReqsizeGiBsAxes(self):
        """@return """
        # Round the reqsize up to the nearest thousand or so
        maxReq = self._getMaxReqsize() / (1024*1024)
        tens = int(floor(log10(maxReq)))
        reqRange = int(ceil(maxReq/pow(10,tens)) * pow(10, tens))
        
        rga = os.linesep
        # Set the X and Y axis labels
        rga += 'set ylabel "GiB/s"' + os.linesep
        rga += 'set xlabel "Req. Size (MiB)"' + os.linesep
        rga += 'set xtics 0,1,64' + os.linesep
        # Set X and Y ranges
        rga += 'set yrange [0:' + str(self._getMaxGiBs() + 1) + ']' + os.linesep
        rga += 'set xrange [0:' + str(reqRange + 1) + ']' + os.linesep
        return rga
    
    def _getThreadsGiBsAxes(self):
        """@return """
        tga = os.linesep
        # Set the X and Y axis labels
        tga += 'set ylabel "GiB/s"' + os.linesep
        tga += 'set xlabel "Threads"' + os.linesep
        tga += 'set xtics 0,1,64' + os.linesep
        # Set X and Y ranges
        tga += 'set yrange [0:' + str(self._getMaxGiBs() + 1) + ']' + os.linesep
        tga += 'set xrange [0:' + str(self._getMaxThreads() + 1) + ']' + os.linesep        
        return tga
    
    def _getMaxIOPs(self):
        """ @return the maximum number of iops"""
        if self._maxIOPs is None:
            conn = sqlite3.connect(self._sqllog)
            c = conn.cursor()
            c.execute('SELECT max(iops) FROM trials')
            self._maxIOPs = float(c.fetchone()[0])
            conn.close()
        return int(ceil(self._maxIOPs))

    def _getMaxThreads(self):
        """ @return the maximum number of threads"""
        if self._maxThreads is None:
            conn = sqlite3.connect(self._sqllog)
            c = conn.cursor()
            c.execute('SELECT max(threads) FROM trials')
            self._maxThreads = int(c.fetchone()[0])
            conn.close()
        return self._maxThreads
    
    def _getMaxGiBs(self):
        """ @return the maximum throughput"""
        if self._maxGiBs is None:
            conn = sqlite3.connect(self._sqllog)
            c = conn.cursor()
            c.execute('SELECT max(GiBs) FROM trials')
            self._maxGiBs = float(c.fetchone()[0])
            conn.close()
        return int(ceil(self._maxGiBs))

    def _getMaxReqsize(self):
        """ @return the maximum number of threads"""
        if self._maxReqsize is None:
            conn = sqlite3.connect(self._sqllog)
            c = conn.cursor()
            c.execute('SELECT max(reqsz) FROM trials')
            self._maxReqsize = int(c.fetchone()[0])
            conn.close()
        return self._maxReqsize
