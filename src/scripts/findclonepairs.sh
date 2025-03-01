#!/bin/bash
# Find all clones in a given set of potential clones in XML format

# Revised 1.10.18

ulimit -s hard

# Find our installation
lib="${0%%/scripts/findclonepairs.sh}"
if [ ! -d ${lib} ]
then
    echo "*** Error:  cannot find NiCad installation ${lib}"
    echo ""
    exit 99
fi

# Check we have a system
if [ ! -f "$1" ] 
then
    echo "Usage:  FindClonePairs system-name_granularity.xml threshold [ minclonesize ] [ maxclonesize] [ showsource ]"
    echo "  e.g.,   FindClonePairs systems/c/linux_functions.xml 0.0"
    echo "  e.g.,   FindClonePairs systems/c/linux_functions.xml 0.2 3 2000 showsource"
    exit 99
else
    pcsfile="$1"
    shift
fi

# And a threshold
if [ "$1" != "" ] 
then
    threshold=$1
else
    echo "Usage:  FindClonePairs system-name_functions.xml threshold [ minclonesize ] [ maxclonesize] [ showsource ]"
    exit 99
fi

# Other arguments we just pass on to clonepairs.x

# Put our results in the same directory as the potential clones file
basename="${pcsfile%%.xml}"
resultsdir="${basename}-clones"
mkdir "${resultsdir}" 2> /dev/null
systemname="${basename##*/}"

# OK, let's run it
date
echo "${lib}/tools/clonepairs.x ${basename}.xml $* > ${resultsdir}/${systemname}-clones-${threshold}.xml"
time ${lib}/tools/clonepairs.x "${basename}.xml" $* > "${resultsdir}/${systemname}-clones-${threshold}.xml"
result=$?

echo ""
date
echo ""

exit $result
