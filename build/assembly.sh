#!/bin/bash

if [[ $(basename $(pwd)) != "build" ]]; then echo "Please run from the 'build' directory!"; exit; fi

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
            Cancel ) unset FPX_ASM_ARCH; exit;;
        esac
    done
fi


> params.fpx
if [ $FPX_ASM_ARCH != "no" ]; then \
    echo "asm:$FPX_ASM_ARCH" >> params.fpx; \
fi
echo ""