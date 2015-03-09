#!/bin/sh

# set -x

if [ -z "$ARCH" ] ;then
    ARCH=`uname -m`
    echo "building for $ARCH"
fi

while getopts a:hx:o: flag; do
    case $flag in
        a)
            ARGS=$OPTARG
            ;;
        h)
            echo "Help."
            printf "\t-a <makefile args>\n"
            printf "\t-h\t\tHelp.\n"
            printf "\t-o <variant>\tBuild a variant based on an architecture\n"
            printf "\t-x <makefile arch>\n"

            printf "\nThe follwing environment files effect behavior:\n\n"
            printf "\tARCH\tBuild for the specified architecture.\n"

            exit 0
            ;;
        o)
            OPT=_${OPTARG}
            ;;
        x)
            ARCH=${OPTARG}
            ;;
    esac
done

MAKEFILE=Makefile.${ARCH}${OPT}

if [ -f $MAKEFILE ]; then
    echo "Building with $MAKEFILE"
	make -j 4 -f $MAKEFILE $ARGS
else
	echo "$MAKEFILE does not exist."
fi
