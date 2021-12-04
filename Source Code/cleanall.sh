#!/bin/bash

clear
echo "\033[33;40mCleaning all programs\033[0m"

for d in ./*/*/ ; do 
(
	cd "$d" 
	echo ""
	echo "\033[34;40mCleaning $d\033[0m"
	make clean
); 
done

cd "./Original/"
echo ""
echo "\033[34;40mCleaning Original Program\033[0m"
make clean

echo ""
echo "\033[32;40mFinished!\033[0m"