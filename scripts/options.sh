#!/bin/bash

PARAMSFILE=$(find . -name "params.fpx" | head -n 1)

if [[ -z "$PARAMSFILE" ]] && [[ "$(basename $PWD)" == "scripts" ]]; then
  touch params.fpx;
elif [[ -z "$PARAMSFILE" ]]; then
  find . -name "scripts" -type d -exec touch {}/params.fpx \;
fi

PARAMSFILE=$(find . -name "params.fpx" | head -n 1)

# echo $PARAMSFILE && echo $LASTTARGET && echo $TARGET

CheckClean() {
  grep -Eq "^last_(.+):(.+)$" $PARAMSFILE;
  if ! [[ $? -eq 0 ]]; then echo "clean"; exit 1; fi

  while read LINE; do
    KV="$(echo "$LINE" | sed -nE 's/^last_(.+):(.+)$/\1 \2/p')";
    if [[ -n "$KV" ]]; then
      KEY="$(echo $KV | awk '{print $1}')"
      VAL="$(echo $KV | awk '{print $2}')"
      if [[ "$VAL" != "$(sed -nE "s/^current_$KEY:(.+)/\1/p" $PARAMSFILE)" ]]; then
        echo "clean";
        exit;
      fi;
    fi;
  done < $PARAMSFILE;

  echo "noclean";
}

SetLast() {
  while read LINE; do
    KV="$(echo "$LINE" | sed -nE 's/^current_(.+):(.+)$/\1 \2/p')";
    if [[ -n "$KV" ]]; then
      KEY="$(echo $KV | awk '{print $1}')";
      VAL="$(echo $KV | awk '{print $2}')";
      grep -Eq "^last_$KEY:(.+)$" $PARAMSFILE;
      if [[ $? -eq 0 ]]; then
        sed -i -r -e "s/^last_$KEY:(.+)$/last_$KEY:$VAL/g" $PARAMSFILE;
      else
        echo "last_$KEY:$VAL" >> $PARAMSFILE;
      fi;
    fi;
  done < $PARAMSFILE;
}

if [[ -n "$1" ]]; then
  $1;
  exit;
fi

if [[ $(basename $(pwd)) != "scripts" ]]; then echo "Please run from the 'scripts' directory!"; exit; fi

if [[ -z "$FPX_MODE" ]]; then echo "Please set FPX_MODE to the used mode (debug, release, etc.)"; exit; fi

echo "Target: $FPX_MODE";

echo -e "\nWould you like to incorporate handwritten assembly for some functions? This can improve performance."
select yn in "Yes" "No"; do
    case $yn in
        Yes ) FPX_ASM_ARCH=yes; break;;
        No ) FPX_ASM_ARCH=no; break;;
    esac
done

if [[ "$FPX_ASM_ARCH" == "yes" ]]; then
    echo -e "\nFor which architecture do you wish to assemble?"
    select arch in "x86_64" "Cancel"; do
        case $arch in
            x86_64 ) FPX_ASM_ARCH=x86_64; break;;
            Cancel ) break;;
        esac
    done
fi

# check architecture
grep -Eq "^current_architecture:(.*)$" params.fpx

if [ $? -eq 0 ]; then
  GREPSUCCESS="true";
else
  GREPSUCCESS="false";
fi

if [ "$GREPSUCCESS" = "true" ] && [ "$FPX_ASM_ARCH" != "no" ]; then # set asm architecture in file
  sed -i -r -e "s/^current_architecture:(.*)$/current_architecture:$FPX_ASM_ARCH/g" params.fpx;
elif [ "$FPX_ASM_ARCH" != "no" ]; then # assembly is true; add line to params file
  echo "current_architecture:$FPX_ASM_ARCH" >> params.fpx;
elif [ "$GREPSUCCESS" = "true" ]; then # assembly is false; replace line in params file
  sed -i -r -e "s/^current_architecture:(.*)$/current_architecture:noasm/g" params.fpx;
else # add assembly target line to params file
  echo "current_architecture:noasm" >> params.fpx;
fi
# end check architecture

# check target
grep -Eq "^current_mode:(.*)$" params.fpx

if [ $? -eq 0 ]; then
  GREPSUCCESS="true";
else
  GREPSUCCESS="false";
fi

if [ "$GREPSUCCESS" = "true" ]; then # set current mode line
  sed -i -r -e "s/^current_mode:(.*)$/current_mode:$FPX_MODE/g" params.fpx;
else # add mode line to params file
  echo "current_mode:$FPX_MODE" >> params.fpx;
fi
# end check target

echo -e "\nChoices saved to 'scripts/params.fpx'\n";
