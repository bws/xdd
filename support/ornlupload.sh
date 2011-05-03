#!/bin/bash

# ornlupload.sh
#
# 1. Takes a filename as an argument.
# 2. Uploads the file to ORNL's file server.
# 3. Prints the URL to the uploaded file
#
# `curl` output is dumped to CURL_OUTPUT,
# in case of a parsing error.

CURL_OUTPUT="ornlupload.curl.out"
FILE_UPLOAD_SCRIPT="http://home.ornl.gov/cgi-bin/cgiwrap/loader/fileupload.cgi"
PUBLIC="no"

function print_usage {
	echo "Usage: $0 [options] file_to_upload"
	echo "Options:"
	echo "    -p    make the file public (available outside ORNL network)"
}

# get command line arguments
while getopts "p" option; do
	case $option in
		p)	
			PUBLIC="yes"
			;;
	esac
done
shift $((OPTIND-1))
if [ $# -ne 1 ]; then
	print_usage
	exit 1;
fi
FILENAME=$1;

# prepend PWD to relative path names
DIRNAME=$(dirname $FILENAME)
if [[ $DIRNAME != /* ]]; then
	FILENAME=$(pwd)/$FILENAME
fi

# make sure file exists
if [ ! -e $FILENAME ]; then
	echo "Invalid filename"
	exit 1;
fi

# send POST to the file upload script, store output in CURL_OUTPUT
curl -s -F "file-to-upload-01=@$FILENAME" -F "make_public=$PUBLIC" $FILE_UPLOAD_SCRIPT > $CURL_OUTPUT

# grep/sed out file URL from CURL_OUTPUT
URL=$(cat $CURL_OUTPUT | grep "&lt;http://.*.ornl.gov/filedownload" | sed "s/&[lg]t;//g" | sed "s/<br>//")

# if the parsing screwed up, print an error and exit
if [[ $URL != http* ]]; then
	echo "Failed to find uploaded file URL. Check $CURL_OUTPUT for details."
	exit 1;
fi

# print file URL and exit nicely
echo $URL
exit 0

