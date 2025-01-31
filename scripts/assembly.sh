#!/bin/bash

if [[ $(basename $(pwd)) != "scripts" ]]; then echo "Please run from the 'scripts' directory!"; exit; fi

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

grep -Eq "^current_target:(.*)$" params.fpx

if [ $? -eq 0 ]; then
  GREPSUCCESS="true";
else
  GREPSUCCESS="false";
fi

if [ "$GREPSUCCESS" = "true" ] && [ "$FPX_ASM_ARCH" != "no" ]; then
  sed -i -r -e "s/^current_target:(.*)$/current_target:$FPX_ASM_ARCH/g" params.fpx;
elif [ "$FPX_ASM_ARCH" != "no" ]; then
  echo "current_target:$FPX_ASM_ARCH" >> params.fpx;
elif [ "$GREPSUCCESS" = "true" ]; then
  sed -i -r -e "s/^current_target:(.*)$/current_target:noasm/g" params.fpx;
else
  echo "current_target:noasm" > params.fpx;
fi
echo -e "\nChoices saved to 'scripts/params.fpx'\n";
