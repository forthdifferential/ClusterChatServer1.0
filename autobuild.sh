#########################################################################
# File Name: autobuild.sh
# Author: forthdifferential
# Created Time: 2024年3月3日21:35:12
#########################################################################
#!/bin/bash

set -x

rm -rf `pwd`/build/*
cd `pwd`/build &&
	cmake .. &&
	make