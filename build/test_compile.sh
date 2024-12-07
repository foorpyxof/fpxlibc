#!/bin/bash

CMD='g++'
i=0
active=false
while read line; do
	if [ "$line" == 'start' ]; then active=true; continue; fi
	if ! $active; then continue; fi
	if [ "$line" == '' ]; then
		if [ "$CMD" == 'g++' ]; then continue; fi
		# echo $CMD
		$CMD
		i=0
		CMD="g++"
	elif [ "$line" == 'end' ]; then
		break
	else
		if [ $i == 0 ]; then
			CMD+=" -o "
		else
			CMD+=" unlinked/"
		fi
		CMD+="$line"
		i+=1		
	fi
done <compile.fpx