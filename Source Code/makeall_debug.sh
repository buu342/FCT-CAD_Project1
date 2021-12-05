#!/bin/bash

clear
echo "\033[33;40mCompiling all programs\033[0m"

for d in ./*/ ; do 
(
	cd "$d" 
	echo ""
	echo "\033[34;40mMaking $d\033[0m"
	make DEBUG=1
); 
done

echo ""
echo "\033[32;40mFinished!\033[0m"