#!/bin/bash

if [ -z $1 ];then 
	echo "usage: conan upload [recipe]"
	exit  0
fi
conan upload $1 -r artifactory --all
