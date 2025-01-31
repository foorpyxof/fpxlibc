#!/bin/bash

PARAMSFILE=$(find . -name "params.fpx" | head -n 1)
LASTTARGET=$(sed -nE 's/^last_target:(.+)$/\1/p' $PARAMSFILE)
TARGET=$(sed -nE 's/^current_target:(.+)$/\1/p' $PARAMSFILE)

# echo $PARAMSFILE && echo $LASTTARGET && echo $TARGET

CheckClean() {
  [ -z "$LASTTARGET" ] && echo "clean" && exit
  [ "$LASTTARGET" != "$TARGET" ] && echo "clean" && exit
  echo "noclean"
}

SetLast() {
  [ -z "$TARGET" ] && exit
  [ -z "$LASTTARGET" ] && echo "last_target:$TARGET" >> $PARAMSFILE && exit
  sed -i -e "s/^last_target:$LASTTARGET$/last_target:$TARGET/g" $PARAMSFILE
}

[ -z "$1" ] && exit 1;

$1
