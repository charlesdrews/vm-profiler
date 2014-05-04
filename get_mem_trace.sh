#!/bin/sh
# produce a memory trace using Valgrind

usage="Usage: $0 <path/target_executable[ options]>"

# check if a target exe was specified
if [ $# -lt 1 ]
then
	echo $usage
	exit 1
fi

# save target exe ($1) and concatenate list of options ($2+)
exe="$1"
shift
for a in "$@"
do
	opts="$opts $a"
done

# create ouput filename
output="$(echo $exe | \
	sed -e 's|^\(.*\)\(\.[^./]*\)$|\1|')""_trace.txt"
	#sed -e 's|^\(.*\)\(\.[^./]*\)$|\1|' \
	#    -e 's|^\([^/]*/\)*\([^./]*\)$|\2|')""_trace.txt"
	# 1st sed removes appended file extension if present
	# 2nd sed removes prepended path if present (don't use for now)

# run Valgrind with Lackey tool
valgrind --log-fd=1 --tool=lackey --trace-mem=yes $exe$opts > $output
if [ $? != 0 ]
then
	rm -f $output
	echo $usage
else
	echo "created $output with $(wc -l $output | cut -d' ' -f1) lines"
fi
