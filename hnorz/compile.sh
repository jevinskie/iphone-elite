#!/bin/sh

echo "Compiling HNOR... "

/usr/local/arm-apple-darwin/bin/arm-apple-darwin-cc -Wall -O0 -c norz.c -o norz.o || exit
/usr/local/arm-apple-darwin/bin/arm-apple-darwin-gcc -Xlinker "-syslibroot" -Xlinker "/usr/local/arm-apple-darwin/heavenly" -framework IOKit norz.o -o hnor || exit

echo
echo OK

