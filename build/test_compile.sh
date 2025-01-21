#!/bin/bash

CMD=$CCPLUS
declare -i i=0
active=false

while read line; do
	if [ "$line" == '>start' ]; then active=true; continue; fi
	if ! $active; then continue; fi
	if [ "$line" == '' ]; then
		if [ "$CMD" == $CCPLUS ]; then continue; fi
		#echo $CMD
    echo $echo_msg
		$CMD
		i=0
		CMD=$CCPLUS
	elif [ "$line" == '>end' ]; then
		break
	else
		if [ $i == 0 ]; then
			CMD+=" -o "
      echo_msg="[LD] ./build/$line"
		else
			CMD+=" unlinked/"
      # if [ $i != 1 ]; then echo_msg+=", "; fi
		fi
		CMD+="$line"
    # echo_msg+="build/$line"
    # if [ $i == 0 ]; then echo_msg+=" : "; fi
		i+=1
	fi
done <compile.fpx
